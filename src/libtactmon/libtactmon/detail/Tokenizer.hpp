#pragma once

#include <algorithm>
#include <cstdint>
#include <limits>
#include <span>
#include <string>
#include <string_view>
#include <type_traits>
#include <vector>

namespace libtactmon::detail {
    /**
     * An iterable used to lazily tokenize a string.
     */
    template <typename T, typename CharT, typename... Args>
    struct Tokenizer final : T {
        static_assert(std::is_constructible_v<T, Args...>);

        explicit Tokenizer(std::basic_string_view<CharT> input, bool ignoreEmptyEntries, Args&&... args) noexcept
            : T(std::forward<Args>(args)...), _input(input), _ignoreEmptyEntries(ignoreEmptyEntries)
        { }

        struct Sentinel final { };

        struct Iterator final : std::forward_iterator_tag {
            Iterator(std::basic_string_view<CharT> token, std::basic_string_view<CharT> next, bool ignoreEmptyEntries, T handler) noexcept
                : _handler(std::move(handler)), _token(token), _next(next), _ignoreEmptyEntries(ignoreEmptyEntries)
            { }

            std::basic_string_view<CharT> operator * () const { return _token; }
            std::basic_string_view<CharT>* operator -> () { return std::addressof(_token); }

            std::basic_string_view<CharT> const* operator -> () const { return std::addressof(_token); }

            Iterator& operator ++ () {
                do {
                    std::tie(_token, _next) = _handler.Next(_next);
                } while (_ignoreEmptyEntries && _token.empty() && !_next.empty());

                return *this;
            }

            Iterator operator ++ (int) {
                Iterator copy { *this };
                ++(*this);
                return copy;
            }

            friend bool operator == (Iterator const& left, Iterator const& right) {
                return left._token == right._token && left._next == right._next;
            }

            friend bool operator != (Iterator const& left, Iterator const& right) {
                return !(left == right);
            }

            friend bool operator == (Iterator const& left, Sentinel const& right) {
                return left._next.empty() && left._token.empty();
            }

            friend bool operator != (Iterator const& left, Sentinel const& right) {
                return !(left == right);
            }

        private:
            const T _handler;
            std::basic_string_view<CharT> _token;
            std::basic_string_view<CharT> _next;
            bool _ignoreEmptyEntries;
        };

        Iterator begin() const {
            auto [token, next] = static_cast<const T*>(this)->Next(_input);
            return Iterator { token, next, _ignoreEmptyEntries, T { *static_cast<const T*>(this) } };
        }

        Sentinel end() const {
            return Sentinel { };
        }

        std::vector<std::basic_string_view<CharT>> Accumulate() && {
            std::vector<std::basic_string_view<CharT>> v;
            for (auto it = begin(); it != end(); ++it)
                v.push_back(*it);
            return v;
        }

    private:
        std::basic_string_view<CharT> _input;
        bool _ignoreEmptyEntries;
    };

    namespace impl {
        using namespace std::string_view_literals;

        template <std::size_t N>
        struct Lit {
            constexpr Lit(const char(&c)[N]) noexcept {
                std::copy_n(c, N, value);
            }

            constexpr std::string_view AsView() const noexcept { return value; }
            constexpr std::size_t Size() const noexcept { return N - 1; }

            char value[N];
        };

        template <Lit Token>
        struct NewlineTokenizer {
            std::pair<std::string_view, std::string_view> Next(std::string_view input) const noexcept {
                std::size_t offset = input.find(Token.AsView());
                if (offset == std::string_view::npos)
                    return std::pair{ input, input.substr(input.size()) };

                return std::pair{ input.substr(0, offset), input.substr(offset + Token.Size()) };
            }
        };

        struct ConfigTokenizer {
            std::pair<std::string_view, std::string_view> Next(std::string_view input) const noexcept {
                std::size_t offset = input.find('|');
                if (offset != std::string_view::npos)
                    return std::pair{ input.substr(0, offset), input.substr(offset + 1) };

                offset = input.find(' ');
                if (offset != std::string_view::npos)
                    return std::pair{ input.substr(0, offset), input.substr(offset + 1) };

                return std::pair{ input, input.substr(input.size()) };
            }
        };

        struct RibbitTokenizer {
            std::pair<std::string_view, std::string_view> Next(std::string_view input) const noexcept {
                std::size_t offset = input.find('|');
                if (offset != std::string_view::npos)
                    return std::pair{ input.substr(0, offset), input.substr(offset + 1) };

                return std::pair{ input, input.substr(input.size()) };
            }
        };

        template <auto C>
        struct CharacterTokenizer {
            std::pair<std::basic_string_view<decltype(C)>, std::basic_string_view<decltype(C)>> Next(std::basic_string_view<decltype(C)> input) const noexcept {
                std::size_t offset = input.find(C);
                if (offset != std::basic_string_view<decltype(C)>::npos)
                    return std::pair { input.substr(0, offset), input.substr(offset + 1) };

                return std::pair { input, input.substr(input.size()) };
            }
        };

        template <typename CharT>
        struct StringTokenizer {
            StringTokenizer(std::string token) : Token(std::move(token)) { }

            std::pair<std::basic_string_view<CharT>, std::basic_string_view<CharT>> Next(std::basic_string_view<CharT> input) const noexcept {
                std::size_t offset = input.find(Token);
                if (offset == std::string_view::npos)
                    return std::pair { input, input.substr(input.size()) };

                return std::pair { input.substr(0, offset), input.substr(offset + Token.size()) };
            }

        private:
            std::string Token;
        };
    }

    /**
     * Tokenizes an input string view according to newlines, yielding an iterator over a sequence of string views.
     *
     * @tparam[in] Token The newline token to use.
     */
    template <impl::Lit Token = "\r\n">
    using NewlineTokenizer = Tokenizer<impl::NewlineTokenizer<Token>, char>;

    /**
     * Tokenizes an input string view, assuming said view represents a TACT configuration file.
     */
    using ConfigurationTokenizer = Tokenizer<impl::ConfigTokenizer, char>;

    /**
     * Tokenizes an input string view, assuming said view represents a Ribbit payload.
     */
    using RibbitTokenizer = Tokenizer<impl::RibbitTokenizer, char>;

    /**
     * Tokenizes an input string view according to a single specific character.
     *
     * @tparam[in] Token The character to tokenize ok.
     */
    template <auto Token>
    using CharacterTokenizer = Tokenizer<impl::CharacterTokenizer<Token>, decltype(Token)>;

    /**
     * Tokenizes an input string view according to a runtime string.
     *
     * @tparam[in] CharT The type of character to tokenize.
     */
    template <typename CharT = char>
    using StringTokenizer = Tokenizer<impl::StringTokenizer<CharT>, CharT, std::string>;
}

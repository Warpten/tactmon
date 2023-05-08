#pragma once

#include <cstdint>
#include <limits>
#include <span>
#include <string_view>
#include <type_traits>
#include <vector>

namespace libtactmon::detail {
    /**
     * An iterable used to lazily tokenize a string.
     */
    template <typename T, typename CharT = char, typename... Args>
    struct Tokenizer final : T {
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
                Iterator copy{ *this };
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
                return left._next.empty();
            }

            friend bool operator != (Iterator const& left, Sentinel const& right) {
                return !(left == right);
            }

        private:
            T _handler;
            std::basic_string_view<CharT> _token;
            std::basic_string_view<CharT> _next;
            bool _ignoreEmptyEntries;
        };

        Iterator begin() {
            auto [token, next] = static_cast<T*>(this)->Next(_input);
            return Iterator { token, next, _ignoreEmptyEntries, T { *static_cast<T*>(this) } };
        }

        Sentinel end() {
            return Sentinel { };
        }

        std::vector<std::basic_string_view<CharT>> Accumulate() && {
            std::vector<std::basic_string_view<CharT>> v;
            for (std::basic_string_view<CharT> token : *this)
                v.push_back(token);
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
            constexpr Lit(const char(&c)[N]) {
                std::copy_n(c, N, value);
            }

            constexpr std::string_view AsView() const { return value; }
            constexpr std::size_t Size() const { return N; }

            char value[N];
        };

        template <Lit Token>
        struct NewlineTokenizer {
            std::pair<std::string_view, std::string_view> Next(std::string_view input) {
                std::size_t offset = input.find(Token.AsView());
                if (offset == std::string_view::npos)
                    return std::pair{ input, input.substr(input.size()) };

                return std::pair{ input.substr(0, offset), input.substr(offset + Token.Size()) };
            }
        };

        struct ConfigTokenizer {
            std::pair<std::string_view, std::string_view> Next(std::string_view input) {
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
            std::pair<std::string_view, std::string_view> Next(std::string_view input) {
                std::size_t offset = input.find('|');
                if (offset != std::string_view::npos)
                    return std::pair{ input.substr(0, offset), input.substr(offset + 1) };

                return std::pair{ input, input.substr(input.size()) };
            }
        };

        template <auto C>
        struct CharacterTokenizer {
            std::pair<std::basic_string_view<decltype(C)>, std::basic_string_view<decltype(C)>> Next(std::basic_string_view<decltype(C)> input) {
                std::size_t offset = input.find(C);
                if (offset != std::basic_string_view<decltype(C)>::npos)
                    return std::pair { input.substr(0, offset), input.substr(offset + 1) };

                return std::pair { input, input.substr(input.size()) };
            }
        };

        template <typename CharT>
        struct StringTokenizer {
            std::string Token;

            std::pair<std::basic_string_view<CharT>, std::basic_string_view<CharT>> Next(std::basic_string_view<CharT> input) {
                std::size_t offset = input.find(Token);
                if (offset == std::string_view::npos)
                    return std::pair { input, input.substr(input.size()) };

                return std::pair { input.substr(0, offset), input.substr(offset + Token.size()) };
            }
        };
    }

    /**
     * Tokenizes an input string view according to newlines, yielding an iterator over a sequence of string views.
     *
     * @tparam[in] Token The newline token to use.
     */
    template <impl::Lit Token = "\r\n">
    using NewlineTokenizer = Tokenizer<impl::NewlineTokenizer<Token>>;

    /**
     * Tokenizes an input string view, assuming said view represents a TACT configuration file.
     */
    using ConfigurationTokenizer = Tokenizer<impl::ConfigTokenizer>;

    /**
     * Tokenizes an input string view, assuming said view represents a Ribbit payload.
     */
    using RibbitTokenizer = Tokenizer<impl::RibbitTokenizer>;

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

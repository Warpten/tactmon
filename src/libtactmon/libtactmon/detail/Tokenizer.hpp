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
    template <typename T, typename CharT = char>
    struct Tokenizer {
        explicit Tokenizer(std::basic_string_view<CharT> input, bool ignoreEmptyEntries = false) noexcept : _input(input), _ignoreEmptyEntries(ignoreEmptyEntries) { }

        struct Sentinel final { };

        struct Iterator final : std::forward_iterator_tag {
            Iterator(std::basic_string_view<CharT> token, std::basic_string_view<CharT> next, bool ignoreEmptyEntries) noexcept
                : _token(token), _next(next), _ignoreEmptyEntries(ignoreEmptyEntries)
            { }

            std::basic_string_view<CharT> operator * () const { return _token; }
            std::basic_string_view<CharT>* operator -> () { return std::addressof(_token); }

            std::basic_string_view<CharT> const* operator -> () const { return std::addressof(_token); }

            Iterator& operator ++ () {
                do {
                    std::tie(_token, _next) = T::Next(_next);
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
            std::basic_string_view<CharT> _token;
            std::basic_string_view<CharT> _next;
            bool _ignoreEmptyEntries;
        };

        Iterator begin() {
            auto [token, next] = T::Next(_input);
            return Iterator { token, next, _ignoreEmptyEntries };
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
        struct NewlineTokenizer final {
            static std::pair<std::string_view, std::string_view> Next(std::string_view input) {
                std::size_t offset = input.find(Token.AsView());
                if (offset == std::string_view::npos)
                    return std::pair{ input, input.substr(input.size()) };

                return std::pair{ input.substr(0, offset), input.substr(offset + Token.Size()) };
            }
        };

        struct ConfigTokenizer final {
            static std::pair<std::string_view, std::string_view> Next(std::string_view input) {
                std::size_t offset = input.find('|');
                if (offset != std::string_view::npos)
                    return std::pair{ input.substr(0, offset), input.substr(offset + 1) };

                offset = input.find(' ');
                if (offset != std::string_view::npos)
                    return std::pair{ input.substr(0, offset), input.substr(offset + 1) };

                return std::pair{ input, input.substr(input.size()) };
            }
        };

        struct RibbitTokenizer final {
            static std::pair<std::string_view, std::string_view> Next(std::string_view input) {
                std::size_t offset = input.find('|');
                if (offset != std::string_view::npos)
                    return std::pair{ input.substr(0, offset), input.substr(offset + 1) };

                return std::pair{ input, input.substr(input.size()) };
            }
        };

        template <auto C>
        struct CharacterTokenizer final {
            static std::pair<std::basic_string_view<decltype(C)>, std::basic_string_view<decltype(C)>> Next(std::basic_string_view<decltype(C)> input) {
                std::size_t offset = input.find(C);
                if (offset != std::basic_string_view<decltype(C)>::npos)
                    return std::pair { input.substr(0, offset), input.substr(offset + 1) };

                return std::pair { input, input.substr(input.size()) };
            }
        };
    }

    template <impl::Lit Token = "\r\n">
    using NewlineTokenizer = Tokenizer<impl::NewlineTokenizer<Token>>;

    using ConfigurationTokenizer = Tokenizer<impl::ConfigTokenizer>;
    using RibbitTokenizer = Tokenizer<impl::RibbitTokenizer>;

    template <auto Token>
    using CharacterTokenizer = Tokenizer<impl::CharacterTokenizer<Token>>;

    template <typename CharT, typename Traits = std::char_traits<CharT>>
    std::vector<std::basic_string_view<CharT, Traits>> Tokenize(
        std::basic_string_view<CharT, Traits> input,
        std::basic_string_view<CharT, Traits> token,
        bool removeEmptyTokens)
    {
        std::vector<std::basic_string_view<CharT, Traits>> result;

        while (!input.empty()) {
            auto pos = input.find(token);
            if (pos == std::basic_string_view<CharT, Traits>::npos) {
                if (!removeEmptyTokens || !input.empty())
                    result.push_back(input);

                break;
            }
            else {
                auto elem = input.substr(0, pos);
                if (!removeEmptyTokens || !elem.empty())
                    result.push_back(elem);

                input.remove_prefix(pos + token.length());
            }
        }

        return result;
    }

    template <typename CharT, typename Traits = std::char_traits<CharT>>
    std::vector<std::basic_string_view<CharT, Traits>> Tokenize(
        std::basic_string_view<CharT, Traits> input,
        std::span<const std::basic_string_view<CharT, Traits>> tokens,
        bool removeEmptyTokens)
    {
        std::vector<std::basic_string_view<CharT, Traits>> result;

        while (!input.empty()) {
            std::basic_string_view<CharT, Traits>* selectedToken = nullptr;
            std::size_t tokenPosition = std::numeric_limits<std::size_t>::max();

            for (std::basic_string_view<CharT, Traits> token : tokens) {
                auto pos = input.find(token);
                if (pos != std::basic_string_view<CharT, Traits>::npos && pos < tokenPosition) {
                    selectedToken = &token;
                    tokenPosition = pos;
                }
            }

            if (selectedToken == nullptr) {
                if (!removeEmptyTokens || !input.empty())
                    result.push_back(input);

                break;
            } else {
                if (!removeEmptyTokens || tokenPosition > 0)
                    result.push_back(input.substr(0, tokenPosition));

                input.remove_prefix(tokenPosition + selectedToken->length());
            }
        }

        return result;
    }

    template <typename CharT, typename Traits = std::char_traits<CharT>>
    std::vector<std::basic_string_view<CharT, Traits>> Tokenize(
        std::basic_string_view<CharT, Traits> input,
        CharT token,
        bool removeEmptyTokens)
    {
        std::vector<std::basic_string_view<CharT, Traits>> result;

        while (!input.empty()) {
            auto pos = input.find(token);
            if (pos == std::basic_string_view<CharT, Traits>::npos) {
                if (!removeEmptyTokens || !input.empty())
                    result.push_back(input);

                break;
            }
            else {
                std::basic_string_view<CharT, Traits> elem = input.substr(0, pos);
                if (!removeEmptyTokens || !elem.empty())
                    result.push_back(elem);

                input.remove_prefix(pos + 1);
            }
        }

        return result;
    }

    template <typename CharT, std::size_t N, typename Traits = std::char_traits<CharT>>
    std::vector<std::basic_string_view<CharT, Traits>> Tokenize(
        std::basic_string_view<CharT, Traits> input,
        std::span<const CharT, N> tokens,
        bool removeEmptyTokens)
    {
        std::vector<std::basic_string_view<CharT, Traits>> result;

        while (!input.empty()) {
            std::size_t tokenPosition = std::numeric_limits<std::size_t>::max();

            for (CharT token : tokens) {
                auto pos = input.find(token);
                if (pos != std::basic_string_view<CharT, Traits>::npos && pos < tokenPosition) {
                    tokenPosition = pos;
                }
            }

            if (tokenPosition == std::numeric_limits<std::size_t>::max()) {
                result.push_back(input);
                break;
            }
            else {
                if (!removeEmptyTokens || tokenPosition > 0)
                    result.push_back(input.substr(0, tokenPosition));
                input.remove_prefix(tokenPosition + 1);
            }
        }

        return result;
    }
}

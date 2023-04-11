#pragma once

#include <cstdint>
#include <limits>
#include <span>
#include <string_view>
#include <vector>

namespace libtactmon::detail {
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

#pragma once

#include <algorithm>
#include <array>
#include <string>
#include <sstream>
#include <utility>

#include "backend/db/orm/Concepts.hpp"

namespace backend::db {
    template <typename T>
    struct IQuery {
        static std::string render() {
            // static_assert(concepts::StreamRenderable<T>);

            std::stringstream ss;
            T::render_to(ss, std::integral_constant<std::size_t, 1> { });
            return ss.str();
        }

        constexpr static auto make_query_string() {
            std::string input = "";
            auto [query, offset] = T::render_to_v2(input, std::integral_constant<std::size_t, 1> { });
            std::array<char, query.size() + 1> arr;
            std::copy_n(query.data(), query.size(), arr.data());
            arr[query.size()] = 0;
            return arr;
        }

        constexpr static const auto QueryString = make_query_string();
    };
}

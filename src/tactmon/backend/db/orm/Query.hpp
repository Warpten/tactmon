#pragma once

#include <algorithm>
#include <array>
#include <sstream>
#include <string>
#include <string_view>
#include <utility>

#include "backend/db/orm/Concepts.hpp"

namespace backend::db {
    template <typename T>
    struct IQuery {
        using parameter_types = typename T::parameter_types;
        using transaction_type = typename T::transaction_type;
        using result_type = typename T::result_type;

    private:
        constexpr static std::string ToString() {
            return T::render_to("", std::integral_constant<std::size_t, 1> { }).first;
        }

        template <std::size_t... Is>
        constexpr static std::array<char, sizeof...(Is) + 1> AsStaticString(std::string const queryString, std::index_sequence<Is...>) {
            return std::array { queryString[Is]..., '\0'};
        }

    public:
        constexpr static const auto AsString = AsStaticString(ToString(), std::make_index_sequence<ToString().size()> { });
    };
}

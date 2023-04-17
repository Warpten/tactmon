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

        static std::string render() {
            // static_assert(concepts::StreamRenderable<T>);

            std::stringstream ss;
            T::render_to(ss, std::integral_constant<std::size_t, 1> { });
            return ss.str();
        }

        static std::string ToString() {
            return T::render_to_v2("", std::integral_constant<std::size_t, 1> { }).first;
        }
    };
}

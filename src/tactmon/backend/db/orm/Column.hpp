#pragma once

#include "backend/db/orm/Concepts.hpp"
#include "utility/Literal.hpp"
#include "utility/Tuple.hpp"

#include <ostream>
#include <type_traits>

namespace backend::db {
    /**
     * Associates a column name and a type.
     *
     * @tparam NAME The column name.
     * @tparam TYPE The type of the column's value.
     */
    template <utility::Literal NAME, typename TYPE>
    struct Column {
        using parameter_types = utility::tuple<>;
        using value_type = TYPE;

        template <std::size_t PARAMETER>
        constexpr static auto render_to(std::string prev, std::integral_constant<std::size_t, PARAMETER> p) {
            return std::make_pair(prev + ' ' + NAME.Value, p);
        }

        /**
         * Binds a column to a projection (or an entity).
         */
        template <typename PROJECTION>
        struct Of {
            using value_type = TYPE;
            using parameter_types = utility::tuple<>;

            template <std::size_t PARAMETER>
            constexpr static auto render_to(std::string prev, std::integral_constant<std::size_t, PARAMETER> p) {
                auto [next, u] = PROJECTION::render_to(prev, p);
                return std::make_pair(next + '.' + NAME.Value, u);
            }

            template <typename> using BindToProjection = Of<PROJECTION>;
        };

        template <typename PROJECTION>
        using BindToProjection = Of<PROJECTION>;
    };
}


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
        static auto render_to(std::ostream& stream, std::integral_constant<std::size_t, PARAMETER> p) {
            stream << NAME.Value;
            return p;
        }

        /**
         * Binds a column to a projection (or an entity).
         */
        template <typename PROJECTION>
        struct Of : Column<NAME, TYPE> {
            template <std::size_t PARAMETER>
            static auto render_to(std::ostream& stream, std::integral_constant<std::size_t, PARAMETER> p);

            template <typename> using BindToProjection = Of<PROJECTION>;
        };

        template <typename PROJECTION>
        using BindToProjection = Of<PROJECTION>;
    };

    template <utility::Literal NAME, typename TYPE>
    template <typename PROJECTION>
    template <std::size_t PARAMETER>
    /* static */ auto Column<NAME, TYPE>::Of<PROJECTION>::render_to(std::ostream& stream, std::integral_constant<std::size_t, PARAMETER> p) {
        auto projectionOffset = PROJECTION::render_to(stream, p);
        stream << '.' << NAME.Value;
        return projectionOffset;
    }
}


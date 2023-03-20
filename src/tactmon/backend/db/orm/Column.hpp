#pragma once

#include "backend/db/orm/concepts/Concepts.hpp"
#include "utility/Literal.hpp"

#include <ostream>
#include <type_traits>

namespace backend::db::orm {
    /**
     * Associates a column name and a type.
     *
     * @tparam NAME The column name.
     * @tparam TYPE The type of the column's value.
     */
    template <utility::Literal NAME, typename TYPE>
    struct Column {
        using value_type = TYPE;

        template <size_t PARAMETER>
        static auto render_to(std::ostream& stream, std::integral_constant<size_t, PARAMETER> p) {
            stream << NAME.Value;
            return p;
        }

        /**
         * Binds a column to a projection (or an entity).
         */
        template <concepts::StreamRenderable PROJECTION>
        struct Of {
            using value_type = TYPE;

            template <size_t PARAMETER>
            static auto render_to(std::ostream& stream, std::integral_constant<size_t, PARAMETER> p);
        };

        template <typename PROJECTION>
        using Bind = Of<PROJECTION>;
    };

    template <utility::Literal NAME, typename TYPE>
    template <concepts::StreamRenderable PROJECTION>
    template <size_t PARAMETER>
    static auto Column<NAME, TYPE>::Of<PROJECTION>::render_to(std::ostream& stream, std::integral_constant<size_t, PARAMETER> p) {
        auto projectionOffset = PROJECTION::render_to(stream, p);
        stream << '.' << NAME.Value;
        return projectionOffset;
    }
}


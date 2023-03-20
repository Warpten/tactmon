#pragma once

#include "backend/db/orm/concepts/Concepts.hpp"
#include "utility/Literal.hpp"

#include <ostream>
#include <utility>

namespace backend::db::orm::select {
    /**
     * A common table expression.
     *
     * @tparam ALIAS     The CTE's name.
     * @tparam RECURSIVE Indicates wether or not this CTE is recursive.
     * @tparam QUERY     The query if this CTE.
     */
    template <utility::Literal ALIAS, bool RECURSIVE, concepts::StreamRenderable QUERY>
    struct CTE {
        template <size_t I>
        static auto render_to(std::ostream& ss, std::integral_constant<size_t, I>);
    };

    template <utility::Literal ALIAS, bool RECURSIVE, concepts::StreamRenderable QUERY>
    template <size_t I>
    /* static */ auto CTE<ALIAS, RECURSIVE, QUERY>::render_to(std::ostream& ss, std::integral_constant<size_t, I> p) {
        if constexpr (RECURSIVE)
            ss << "RECURSIVE ";

        ss << ALIAS.Value << " AS (";
        auto cteOffset = QUERY::render_to(ss, p);
        ss << ')';

        return cteOffset;
    };
}

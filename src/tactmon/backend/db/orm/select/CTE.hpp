#pragma once

#include "backend/db/orm/Concepts.hpp"
#include "utility/Literal.hpp"

#include <ostream>
#include <utility>

namespace backend::db::select {
    /**
     * A common table expression.
     *
     * @tparam ALIAS     The CTE's name.
     * @tparam RECURSIVE Indicates wether or not this CTE is recursive.
     * @tparam QUERY     The query if this CTE.
     */
    template <utility::Literal ALIAS, bool RECURSIVE, typename QUERY>
    struct CTE final {
        using parameter_types = typename QUERY::parameter_types;

        template <std::size_t I>
        static auto render_to(std::ostream& ss, std::integral_constant<std::size_t, I>);

        template <std::size_t PARAMETER>
        constexpr static auto render_to_v2(std::string prev, std::integral_constant<std::size_t, PARAMETER> p) {
            if constexpr (RECURSIVE) {
                auto [next, u] = QUERY::render_to_v2(prev + "RECURSIVE " + ALIAS.Value + " AS (", p);
                return std::make_pair(next + ")", u);
            }
            else {
                auto [next, u] = QUERY::render_to_v2(prev + ALIAS.Value + " AS (", p);
                return std::make_pair(next + ")", u);
            }
        }
    };

    template <utility::Literal ALIAS, bool RECURSIVE, typename QUERY>
    template <std::size_t I>
    /* static */ auto CTE<ALIAS, RECURSIVE, QUERY>::render_to(std::ostream& ss, std::integral_constant<std::size_t, I> p) {
        if constexpr (RECURSIVE)
            ss << "RECURSIVE ";

        ss << ALIAS.Value << " AS (";
        auto cteOffset = QUERY::render_to(ss, p);
        ss << ')';

        return cteOffset;
    };
}

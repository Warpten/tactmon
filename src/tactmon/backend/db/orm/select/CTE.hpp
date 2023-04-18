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
}

#pragma once

#include "backend/db/orm/Concepts.hpp"
#include "backend/db/orm/detail/VariadicRenderable.hpp"

#include <cstdint>
#include <utility>

namespace backend::db::orm::update {
    /**
     * The SET component of an UPDATE query.
     *
     * @tparam ASSIGNMENTS... Assignment expression components for each column of the table that is to be modified by this query.
     */
    template <concepts::StreamRenderable... ASSIGNMENTS>
    struct Set final {
        template <size_t PARAMETER>
        static auto render_to(std::ostream& ss, std::integral_constant<size_t, PARAMETER> p) {
            ss << "SET ";
            return detail::VariadicRenderable<", ", ASSIGNMENTS...>::render_to(ss, p);
        }
    };
}

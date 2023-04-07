#pragma once

#include "backend/db/orm/Concepts.hpp"
#include "backend/db/orm/detail/VariadicRenderable.hpp"

#include <cstdint>
#include <utility>

namespace backend::db::update {
    /**
     * The SET component of an UPDATE query.
     *
     * @tparam ASSIGNMENTS... Assignment expression components for each column of the table that is to be modified by this query.
     */
    template <typename... ASSIGNMENTS>
    struct Set final {
        using parameter_types = decltype(utility::tuple_cat(
            std::declval<typename ASSIGNMENTS::parameter_types>()...
        ));

        template <std::size_t PARAMETER>
        static auto render_to(std::ostream& ss, std::integral_constant<std::size_t, PARAMETER> p) {
            ss << "SET ";
            return detail::VariadicRenderable<", ", ASSIGNMENTS...>::render_to(ss, p);
        }
    };
}

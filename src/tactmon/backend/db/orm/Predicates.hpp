#pragma once

#include "backend/db/orm/concepts/Concepts.hpp"
#include "utility/Literal.hpp"

#include <ostream>
#include <utility>

/**
 * This file provides various SQL conditions.
 */

namespace backend::db::orm {
    namespace detail {
        /**
         * Base implementation of a binary predicate.
         */
        template <utility::Literal TOKEN, concepts::StreamRenderable COMPONENT, concepts::StreamRenderable PARAMETER>
        struct BinaryCriteria {
            using parameter_type = typename PARAMETER::template Bind<typename COMPONENT::value_type>;

            template <size_t P>
            static auto render_to(std::ostream& ss, std::integral_constant<size_t, P> p) {
                auto componentOffset = COMPONENT::render_to(ss, p);
                ss << TOKEN.Value;
                return parameter_type::render_to(ss, componentOffset);
            }
        };

        /**
         * Base implementation of an unary predicate.
         */
        template <utility::Literal BEGIN, utility::Literal END, concepts::StreamRenderable CRITERIA>
        struct UnaryCriteria {
            using parameter_type = typename CRITERIA::parameter_type;

            template <size_t P>
            static auto render_to(std::ostream& ss, std::integral_constant<size_t, P> p) {
                ss << BEGIN.Value;
                auto componentOffset = CRITERIA::render_to(ss, p);
                ss << END.Value;
                return componentOffset;
            }
        };
    }

    /**
     * Renders `${COMPONENT} = ${PARAMETER}`.
     */
    template <concepts::StreamRenderable COMPONENT, concepts::StreamRenderable PARAMETER>
    using Equals = detail::BinaryCriteria<" = ", COMPONENT, PARAMETER>;

    /**
     * Renders `${COMPONENT} > ${PARAMETER}`.
     */
    template <concepts::StreamRenderable COMPONENT, concepts::StreamRenderable PARAMETER>
    using GreaterThan = detail::BinaryCriteria<" > ", COMPONENT, PARAMETER>;

    /**
     * Renders `${COMPONENT} < ${PARAMETER}`.
     */
    template <concepts::StreamRenderable COMPONENT, concepts::StreamRenderable PARAMETER>
    using LessThan = detail::BinaryCriteria<" < ", COMPONENT, PARAMETER>;

    /**
     * Renders `!(CRITERIA)`, the logical negation of a nested criteria or boolean-like.
     */
    template <concepts::StreamRenderable CRITERIA>
    using Not = detail::UnaryCriteria<"!(", ")", CRITERIA>;

    /**
     * Renders `${COMPONENT} IS NULL`.
     */
    template <concepts::StreamRenderable COMPONENT>
    using IsNull = detail::UnaryCriteria<"(", " IS NULL)", COMPONENT>;
}

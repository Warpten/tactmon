#pragma once

#include "backend/db/orm/Concepts.hpp"
#include "utility/Literal.hpp"

#include <ostream>
#include <utility>

/**
 * This file provides various SQL conditions.
 */

namespace backend::db {
    namespace detail {
        /**
         * Base implementation of a binary predicate.
         */
        template <utility::Literal TOKEN, typename COMPONENT, typename PARAMETER>
        struct BinaryCriteria {
            using bound_parameter_type = typename PARAMETER::template BindToProjection<COMPONENT>;

            using parameter_types = decltype(utility::tuple_cat(
                std::declval<typename COMPONENT::parameter_types>(),
                std::declval<typename bound_parameter_type::parameter_types>()
            ));

            template <std::size_t P>
            static auto render_to(std::ostream& ss, std::integral_constant<std::size_t, P> p) {
                auto componentOffset = COMPONENT::render_to(ss, p);
                ss << TOKEN.Value;
                return bound_parameter_type::render_to(ss, componentOffset);
            }

            template <std::size_t PARAMETER>
            constexpr static auto render_to_v2(std::string prev, std::integral_constant<std::size_t, PARAMETER> p) {
                auto [next, u] = COMPONENT::render_to_v2(prev, p);
                return bound_parameter_type::render_to_v2(next + TOKEN.Value, u);
            }
        };

        /**
         * Base implementation of an unary predicate.
         */
        template <utility::Literal BEGIN, utility::Literal END, typename CRITERIA>
        struct UnaryCriteria final {
            using parameter_types = typename CRITERIA::parameter_types;

            template <std::size_t P>
            static auto render_to(std::ostream& ss, std::integral_constant<std::size_t, P> p) {
                ss << BEGIN.Value;
                auto componentOffset = CRITERIA::render_to(ss, p);
                ss << END.Value;
                return componentOffset;
            }

            template <std::size_t PARAMETER>
            constexpr static auto render_to_v2(std::string prev, std::integral_constant<std::size_t, PARAMETER> p) {
                auto [next, u] = CRITERIA::render_to_v2(prev + BEGIN.Value, p);
                return std::make_pair(next + END.Value, u);
            }
        };
    }

    /**
     * A SQL equality test.
     * Renders `${COMPONENT} = ${PARAMETER}`.
     */
    template <typename COMPONENT, typename PARAMETER>
    using Equals = detail::BinaryCriteria<" = ", COMPONENT, PARAMETER>;

    /**
     * Renders `${COMPONENT} > ${PARAMETER}`.
     */
    template <typename COMPONENT, typename PARAMETER>
    using GreaterThan = detail::BinaryCriteria<" > ", COMPONENT, PARAMETER>;

    /**
     * Renders `${COMPONENT} < ${PARAMETER}`.
     */
    template <typename COMPONENT, typename PARAMETER>
    using LessThan = detail::BinaryCriteria<" < ", COMPONENT, PARAMETER>;

    /**
     * Renders `!(CRITERIA)`, the logical negation of a nested criteria or boolean-like.
     */
    template <typename CRITERIA>
    using Not = detail::UnaryCriteria<"!(", ")", CRITERIA>;

    /**
     * Renders `${COMPONENT} IS NULL`.
     */
    template <typename COMPONENT>
    using IsNull = detail::UnaryCriteria<"(", " IS NULL)", COMPONENT>;
}

#pragma once

#include "backend/db/orm/concepts/Concepts.hpp"
#include "utility/Tuple.hpp"

#include <ostream>
#include <utility>

/**
 * Shared statement components.
 *
 * 1. WHERE clause
 * 2. Conjunction clause ($1 AND $2 AND ...)
 * 3. Disjunction clause ($1 OR $2 OR ...)
 * 4. Parameters
 */

namespace backend::db::orm {
    /**
     * A WHERE clause.
     *
     * @tparam COMPONENT A predicate to use in a query.
     */
    template <concepts::StreamRenderable COMPONENT>
    struct Where {
        template <size_t I>
        static auto render_to(std::ostream& ss, std::integral_constant<size_t, I> p) {
            ss << "WHERE ";
            return COMPONENT::render_to(ss, p);
        }
    };

    /**
     * A variadic predicate that renders the conjunction if an indefinite amount of predicate.
     *
     * @tparam COMPONENTS... All predicates conjoined in this predicate.
     */
    template <concepts::StreamRenderable... COMPONENTS>
    class Conjunction final {
        template <size_t PARAMETER>
        static auto render_to(std::ostream& ss, std::integral_constant<size_t, PARAMETER> p) {
            ss << '(';
            auto predicateOffset = VariadicRenderable<" AND ", COMPONENTS...>::render_to(ss, p);
            ss << ')';
            return predicateOffset;
        }
    };

    /**
    * A variadic predicate that renders the disjunction if an indefinite amount of predicate.
    *
    * @tparam COMPONENTS... All predicates disjoined in this predicate.
    */
    template <concepts::StreamRenderable... COMPONENTS>
    class Disjunction final {
        template <size_t PARAMETER>
        static auto render_to(std::ostream& ss, std::integral_constant<size_t, PARAMETER> p) {
            ss << '(';
            auto predicateOffset = VariadicRenderable<" OR ", COMPONENTS...>::render_to(ss, p);
            ss << ')';
            return predicateOffset;
        }
    };

    /**
     * A positional SQL parameter.
     */
    struct Parameter {
        /**
         * Do not use this type directly; it will be automatically used when using the parent type inside of criteria declaration.
         */
        template <typename T>
        struct Bind {
            using argument_type = T;

            template <size_t I>
            static auto render_to(std::ostream& ss, std::integral_constant<size_t, I>) {
                ss << '$' << I;

                return std::integral_constant<size_t, I + 1> { };
            }
        };
    };

    /**
     * A positional SQL parameter with a known index. Use this when a positional parameter needs to be used multiple times.
     *
     * @tparam I The parameter's position in the parameter array.
     */
    template <size_t I>
    struct BoundParameter {
        template <typename T>
        struct Bind {
            template <size_t X>
            static auto render_to(std::ostream& ss, std::integral_constant<size_t, X> p) {
                static_assert(I < X, "Unable to bind to a position parameter prior to its first use.");

                ss << '$' << I;
                return p;
            }
        };
    };

    /**
     * Wraps around a constant. Useful for dynamic conditions such as dynamic limits:
     * {@code .cpp}
     * Limit<Constant<10>>; // LIMIT 10
     * {@endcode}
     */
    template <auto V>
    struct Constant {
        template <size_t I>
        static auto render_to(std::ostream& ss, std::integral_constant<size_t, I> p) {
            ss << V;
            return p;
        }
    };

    /**
     * An SQL OFFSET statement. Offsets the database's returned data set by a given amount as specified by the provided component.
     */
    template <typename COMPONENT>
    struct Offset {
        template <size_t I>
        static auto render_to(std::ostream& ss, std::integral_constant<size_t, I> p) {
            ss << "OFFSET ";
            return COMPONENT::render_to(ss, p);
        }
    };

    /**
     * An SQL LIMIT statement. Limits the amount of rows returned by the query as specified by the provided component.
     */
    template <concepts::StreamRenderable COMPONENT>
    struct Limit {
        template <size_t I>
        static auto render_to(std::ostream& ss, std::integral_constant<size_t, I> p) {
            ss << "LIMIT ";
            return COMPONENT::render_to(ss, p);
        }
    };

    /**
     * An element of an ORDER BY clause. Specifies the ordering based on a specific component.
     */
    template <concepts::StreamRenderable COMPONENT, bool ASCENDING = true>
    struct OrderClause final {
        template <size_t PARAMETER>
        static auto render_to(std::ostream& ss, std::integral_constant<size_t, PARAMETER> p) {
            auto componentOffset = COMPONENT::render_to(ss, p);
            ss << (ASCENDING ? " ASC" : " DESC");
            return componentOffset;
        }
    };

    namespace detail {
        template <typename T> struct IsOrderClause : std::false_type { };
        template <typename COMPONENT, bool ASCENDING> struct IsOrderClause<OrderClause<COMPONENT, ASCENDING>> : std::true_type { };
    }

    template <typename T>
    concept IsOrderClause = detail::IsOrderClause<T>::value;

    /**
     * An ORDER BY clause. Specifies ordering based on a varying number of components.
     *
     * @tparam COMPONENTS...
     */
    template <concepts::StreamRenderable... COMPONENTS>
    struct OrderBy final {
        template <size_t I>
        static auto render_to(std::ostream& ss, std::integral_constant<size_t, I> p) {
            ss << "ORDER BY ";
            return VariadicRenderable<", ", COMPONENTS...>::render_to(ss, p);
        }
    };

    /**
     * A GROUP BY clause. Specifies grouping based on a varying number of components.
     *
     * @tparam COMPONENTS... Components that specify the grouping criterias.
     */
    template <concepts::StreamRenderable... COMPONENTS>
    struct GroupBy final {
        template <size_t I>
        static auto render_to(std::ostream& ss, std::integral_constant<size_t, I> p) {
            ss << "GROUP BY ";
            return VariadicRenderable<", ", COMPONENTS...>::render_to(ss, p);
        }
    };
}

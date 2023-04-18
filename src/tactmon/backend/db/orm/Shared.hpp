#pragma once

#include "backend/db/orm/Concepts.hpp"
#include "utility/Tuple.hpp"

#include <ostream>
#include <utility>

namespace backend::db {
    namespace detail {
        template <uint8_t... Digits>
        struct ToChars {
            constexpr static const char Value[] = { ('0' + Digits)..., '\0' };
        };

        template <std::size_t Remainder, std::size_t... Digits>
        struct Explode : Explode<Remainder / 10, Remainder % 10, Digits...> { };

        template <std::size_t... Digits>
        struct Explode<0, Digits...> : ToChars<Digits...> { };
    }

    /**
     * A WHERE clause.
     *
     * @tparam COMPONENT A predicate to use in a query.
     */
    template <typename COMPONENT>
    struct Where final {
        using parameter_types = typename COMPONENT::parameter_types;

        template <std::size_t PARAMETER>
        constexpr static auto render_to_v2(std::string prev, std::integral_constant<std::size_t, PARAMETER> p) {
            return COMPONENT::render_to_v2(prev + "WHERE ", p);
        }
    };

    /**
     * A variadic predicate that renders the conjunction if an indefinite amount of predicate.
     *
     * @tparam COMPONENTS... All predicates conjoined in this predicate.
     */
    template <typename... COMPONENTS>
    struct Conjunction final {
        using parameter_types = decltype(utility::tuple_cat(
            std::declval<typename COMPONENTS::parameter_types>()...
        ));

        template <std::size_t PARAMETER>
        constexpr static auto render_to_v2(std::string prev, std::integral_constant<std::size_t, PARAMETER> p) {
            auto [next, u] = detail::VariadicRenderable<" AND ", COMPONENTS...>::render_to_v2(prev + '(', p);
            return std::make_pair(next + ')', u);
        }
    };

    /**
    * A variadic predicate that renders the disjunction if an indefinite amount of predicate.
    *
    * @tparam COMPONENTS... All predicates disjoined in this predicate.
    */
    template <typename... COMPONENTS>
    struct Disjunction final {
        using parameter_types = decltype(utility::tuple_cat(
            std::declval<typename COMPONENTS::parameter_types>()...
        ));

        template <std::size_t PARAMETER>
        constexpr static auto render_to_v2(std::string prev, std::integral_constant<std::size_t, PARAMETER> p) {
            auto [next, u] = detail::VariadicRenderable<" OR ", COMPONENTS...>::render_to_v2(prev + '(', p);
            return std::make_pair(next + ')', u);
        }
    };

    /**
     * A positional SQL parameter.
     */
    struct Parameter final {
        /**
         * Do not use this type directly; it will be automatically used when using the parent type inside of criteria declaration.
         */
        template <typename COMPONENT>
        struct BindToProjection {
            using parameter_types = utility::tuple<typename COMPONENT::value_type>;

            template <std::size_t PARAMETER>
                constexpr static auto render_to_v2(std::string prev, std::integral_constant<std::size_t, PARAMETER> p) {
                return std::make_pair(prev + "$" + detail::Explode<PARAMETER>::Value, std::integral_constant<std::size_t, PARAMETER + 1> { });
            }
        };
    };

    /**
     * A positional SQL parameter with a known index. Use this when a positional parameter needs to be used multiple times.
     *
     * @tparam I The parameter's position in the parameter array.
     */
    template <std::size_t I>
    struct BoundParameter {
        template <typename COMPONENT>
        struct BindToProjection {
            using parameter_types = utility::tuple<>;

            template <std::size_t PARAMETER>
            constexpr static auto render_to_v2(std::string prev, std::integral_constant<std::size_t, PARAMETER> p) {
                static_assert(I < PARAMETER, "Unable to bind to a position parameter prior to its first use.");

                return std::make_pair(prev + '$' + detail::Explode<I>::Value, p);
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
    struct Constant final {
        using parameter_types = utility::tuple<>;

        template <std::size_t PARAMETER>
        constexpr static auto render_to_v2(std::string prev, std::integral_constant<std::size_t, PARAMETER> p) {
            return std::make_pair(prev + detail::Explode<V>::Value, p);
        }
    };

    /**
     * An SQL OFFSET statement. Offsets the database's returned data set by a given amount as specified by the provided component.
     */
    template <typename COMPONENT>
    struct Offset final {
        using parameter_types = typename COMPONENT::parameter_types;

        template <std::size_t PARAMETER>
        constexpr static auto render_to_v2(std::string prev, std::integral_constant<std::size_t, PARAMETER> p) {
            return COMPONENT::render_to_v2(prev + "OFFSET ", p);
        }
    };

    /**
     * An SQL LIMIT statement. Limits the amount of rows returned by the query as specified by the provided component.
     */
    template <typename COMPONENT>
    struct Limit final {
        using parameter_types = typename COMPONENT::parameter_types;

        template <std::size_t PARAMETER>
        constexpr static auto render_to_v2(std::string prev, std::integral_constant<std::size_t, PARAMETER> p) {
            return COMPONENT::render_to_v2(prev + "LIMIT ", p);
        }
    };

    /**
     * An element of an ORDER BY clause. Specifies the ordering based on a specific component.
     */
    template <typename COMPONENT, bool ASCENDING = true>
    struct OrderClause final {
        using parameter_types = typename COMPONENT::parameter_types;

        template <std::size_t PARAMETER>
        constexpr static auto render_to_v2(std::string prev, std::integral_constant<std::size_t, PARAMETER> p) {
            auto [next, u] = COMPONENT::render_to_v2(prev, p);
            return std::make_pair(ASCENDING ? next + " ASC" : next + " DESC", u);
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
    template <typename... COMPONENTS>
    struct OrderBy final {
        using parameter_types = decltype(utility::tuple_cat(std::declval<typename COMPONENTS::parameter_types>()...));

        template <std::size_t PARAMETER>
        constexpr static auto render_to_v2(std::string prev, std::integral_constant<std::size_t, PARAMETER> p) {
            return detail::VariadicRenderable<", ", COMPONENTS...>::render_to_v2(prev + "ORDER BY ", p);
        }
    };

    /**
     * A GROUP BY clause. Specifies grouping based on a varying number of components.
     *
     * @tparam COMPONENTS... Components that specify the grouping criterias.
     */
    template <typename... COMPONENTS>
    struct GroupBy final {
        using parameter_types = decltype(utility::tuple_cat(std::declval<typename COMPONENTS::parameter_types>()...));

        template <std::size_t PARAMETER>
        constexpr static auto render_to_v2(std::string prev, std::integral_constant<std::size_t, PARAMETER> p) {
            return detail::VariadicRenderable<", ", COMPONENTS...>::render_to_v2(prev + "GROUP BY ", p);
        }
    };
}

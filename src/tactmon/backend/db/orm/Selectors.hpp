#pragma once

#include "backend/db/orm/Concepts.hpp"
#include "backend/db/orm/detail/VariadicRenderable.hpp"

namespace backend::db {
    namespace detail2 { // Duplicated from Shared.hpp, need to move it somewhere else
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
     * Base implementation of a SQL function.
     *
     * @tparam TYPE          The function's return type.
     * @tparam NAME          The name of the function.
     * @tparam COMPONENTS... Components describing the arguments passed to the function.
     */
    template <typename TYPE, utility::Literal NAME, typename... COMPONENTS>
    struct Function final {
        using parameter_types = decltype(utility::tuple_cat(std::declval<typename COMPONENTS::parameter_types>()...));
        using value_type = TYPE;

        template <std::size_t I>
        constexpr static auto render_to_v2(std::string prev, std::integral_constant<std::size_t, I> p) {
            auto [next, u] = detail::VariadicRenderable<", ", COMPONENTS...>::render_to_v2(prev + NAME.Value + '(', p);
            return std::make_pair(next + ')', u);
        }
    };

    template <typename COMPONENT> using Count = Function<uint64_t, "COUNT", COMPONENT>;
    template <typename COMPONENT> using Max = Function<typename COMPONENT::value_type, "MAX", COMPONENT>;
    template <typename COMPONENT> using Min = Function<typename COMPONENT::value_type, "MIN", COMPONENT>;
    template <typename COMPONENT> using Any = Function<typename COMPONENT::value_type, "ANY", COMPONENT>;
    template <typename COMPONENT> using Sum = Function<typename COMPONENT::value_type, "SUM", COMPONENT>;

    /**
     * Basic implementation of mutators on a given component. See below for use cases.
     */
    template <utility::Literal TOKEN, typename COMPONENT>
    struct Selector {
        using parameter_types = typename COMPONENT::parameter_types;
        using value_type = typename COMPONENT::value_type;

        template <std::size_t I>
        constexpr static auto render_to_v2(std::string prev, std::integral_constant<std::size_t, I> p) {
            return COMPONENT::render_to_v2(prev + TOKEN.Value + ' ', p);
        }
    };

    /**
     * Implements the DISTINCT column_name concept.
     */
    template <typename COMPONENT>
    struct Distinct final : Selector<"DISTINCT", COMPONENT> {
        /**
         * Special case of DISTINCT where the uniquity column differs from the column returned.
         */
        template <typename CRITERIA>
        struct On {
            using value_type = typename COMPONENT::value_type;

            template <std::size_t I>
            constexpr static auto render_to_v2(std::string prev, std::integral_constant<std::size_t, I> p) {
                auto [next, u] = CRITERIA::render_to_v2(prev + "DISTINCT ON (", p);
                return COMPONENT::render_to_v2(next + ") ", u);
            }
        };
    };

    /**
     * Aliases a component to a name: `${COMPONENT} AS ${TOKEN}`.
     *
     * @tparam TOKEN     The aliased name applied to a component.
     * @tparam COMPONENT A component that will be aliased to a name.
     */
    template <utility::Literal TOKEN, typename COMPONENT>
    struct Alias {
        using parameter_types = typename COMPONENT::parameter_types;
        using value_type = typename COMPONENT::value_type;

        template <std::size_t I>
        constexpr static auto render_to_v2(std::string prev, std::integral_constant<std::size_t, I> p) {
            auto [next, u] = COMPONENT::render_to_v2(prev, p);
            return std::make_pair(next + " AS " + TOKEN.Value, u);
        }

        /**
         * A reference to the alias.
         */
        struct Reference final {
            using parameter_types = utility::tuple<>;

            template <std::size_t I>
            constexpr static auto render_to_v2(std::string prev, std::integral_constant<std::size_t, I> p) {
                return std::make_pair(prev + TOKEN.Value, p);
            }
        };
    };

    namespace detail {
        template <utility::Literal KEYWORD>
        struct RawLiteral {
            template <std::size_t I>
            constexpr static auto render_to_v2(std::string prev, std::integral_constant<std::size_t, I> p) {
                return std::make_pair(prev + KEYWORD.Value, p);
            }
        };

        template <auto V>
        struct Raw {
            template <std::size_t I>
            constexpr static auto render_to_v2(std::string prev, std::integral_constant<std::size_t, I> p) {
                return std::make_pair(prev + detail2::Explode<V>::Value, p);
            }
        };
    }

    template <auto KEYWORD>
    using Raw = std::conditional_t<
        utility::concepts::IsLiteral<KEYWORD>,
        detail::RawLiteral<KEYWORD>,
        detail::Raw<KEYWORD>
    >;

    template <typename COMPONENT, typename PARTITION>
    struct Over final {
        using value_type = typename COMPONENT::value_type;
        using parameter_types = decltype(utility::tuple_cat(
            std::declval<typename COMPONENT::parameter_types>(),
            std::declval<typename PARTITION::parameter_types>()
        ));

        template <std::size_t I>
        constexpr static auto render_to_v2(std::string prev, std::integral_constant<std::size_t, I> p) {
            auto [next, u] = COMPONENT::render_to_v2(prev, p);
            auto [next2, u2] = PARTITION::render_to_v2(next + " OVER (", u);
            return std::make_pair(next2 + ')', u2);
        }
    };

    template <typename COMPONENT>
    struct PartitionBy final {
        using parameter_types = typename COMPONENT::parameter_types;

        template <std::size_t I>
        constexpr static auto render_to_v2(std::string prev, std::integral_constant<std::size_t, I> p) {
            return COMPONENT::render_to_v2(prev + "PARTITION BY ", p);
        }
    };
}

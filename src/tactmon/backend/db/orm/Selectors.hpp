#pragma once

#include "backend/db/orm/Concepts.hpp"
#include "backend/db/orm/detail/VariadicRenderable.hpp"

namespace backend::db {
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

        template <std::size_t PARAMETER>
        static auto render_to(std::ostream& stream, std::integral_constant<std::size_t, PARAMETER> p) {
            stream << NAME.Value << '(';
            auto result = detail::VariadicRenderable<", ", COMPONENTS...>::render_to(stream, p);
            stream << ')';
            return result;
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
        static auto render_to(std::ostream& stream, std::integral_constant<std::size_t, I> p) {
            stream << TOKEN.Value << ' ';
            return COMPONENT::render_to(stream, p);
        }
    };

    /**
     * Implements the DISTINCT column_name concept.
     */
    template <typename COMPONENT>
    struct Distinct : Selector<"DISTINCT", COMPONENT> {
        /**
         * Special case of DISTINCT where the uniquity column differs from the column returned.
         */
        template <typename CRITERIA>
        struct On {
            using value_type = typename COMPONENT::value_type;

            template <std::size_t I>
            static auto render_to(std::ostream& stream, std::integral_constant<std::size_t, I> p) {
                stream << "DISTINCT ON (";
                auto criteriaOffset = CRITERIA::render_to(stream, p);
                stream << ") ";
                return COMPONENT::render_to(stream, criteriaOffset);
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
        static auto render_to(std::ostream& stream, std::integral_constant<std::size_t, I> p) {
            auto result = COMPONENT::render_to(stream, p);
            stream << " AS " << TOKEN.Value;
            return result;
        }

        /**
         * A reference to the alias.
         */
        struct Reference {
            using parameter_types = utility::tuple<>;

            template <std::size_t I>
            static auto render_to(std::ostream& stream, std::integral_constant<std::size_t, I> p) {
                stream << TOKEN.Value;
                return p;
            }
        };
    };

    namespace detail {
        template <utility::Literal KEYWORD>
        struct RawLiteral {
            template <std::size_t I>
            static auto render_to(std::ostream& ss, std::integral_constant<std::size_t, I> p) {
                ss << KEYWORD.Value;
                return p;
            }
        };

        template <auto V>
        struct Raw {
            template <std::size_t I>
            static auto render_to(std::ostream& ss, std::integral_constant<std::size_t, I> p) {
                ss << V;
                return p;
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
        static auto render_to(std::ostream& ss, std::integral_constant<std::size_t, I> p) {
            auto componentOffset = COMPONENT::render_to(ss, p);
            ss << " OVER (";
            auto result = PARTITION::render_to(ss, componentOffset);
            ss << ')';
            return result;
        }
    };

    template <typename COMPONENT>
    struct PartitionBy final {
        using parameter_types = typename COMPONENT::parameter_types;

        template <std::size_t I>
        static auto render_to(std::ostream& ss, std::integral_constant<std::size_t, I> p) {
            ss << "PARTITION BY ";
            return COMPONENT::render_to(ss, p);
        }
    };
}

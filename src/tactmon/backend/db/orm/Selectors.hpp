#pragma once

#include "backend/db/orm/concepts/Concepts.hpp"
#include "backend/db/orm/VariadicRenderable.hpp"

namespace backend::db::orm {
    /**
     * Base implementation of a SQL function.
     *
     * @tparam TYPE          The function's return type.
     * @tparam NAME          The name of the function.
     * @tparam COMPONENTS... Components describing the arguments passed to the function.
     */
    template <typename TYPE, utility::Literal NAME, concepts::StreamRenderable... COMPONENTS>
    struct Function final {
        using value_type = TYPE;

        template <size_t PARAMETER>
        static auto render_to(std::ostream& stream, std::integral_constant<size_t, PARAMETER> p) {
            stream << NAME.Value << '(';
            auto result = VariadicRenderable<", ", COMPONENTS...>::render_to(stream, p);
            stream << ')';
            return result;
        }
    };

    template <concepts::StreamRenderable COMPONENT> using Count = Function<uint64_t, "COUNT", COMPONENT>;
    template <concepts::StreamRenderable COMPONENT> using Max = Function<typename COMPONENT::value_type, "MAX", COMPONENT>;
    template <concepts::StreamRenderable COMPONENT> using Min = Function<typename COMPONENT::value_type, "MIN", COMPONENT>;
    template <concepts::StreamRenderable COMPONENT> using Any = Function<typename COMPONENT::value_type, "ANY", COMPONENT>;
    template <concepts::StreamRenderable COMPONENT> using Sum = Function<typename COMPONENT::value_type, "SUM", COMPONENT>;

    /**
     * Basic implementation of mutators on a given component. See below for use cases.
     */
    template <utility::Literal TOKEN, concepts::StreamRenderable COMPONENT>
    struct Selector {
        using value_type = typename COMPONENT::value_type;

        template <size_t I>
        static auto render_to(std::ostream& stream, std::integral_constant<size_t, I> p) {
            stream << TOKEN.Value << ' ';
            return COMPONENT::render_to(stream, p);
        }
    };

    /**
     * Implements the DISTINCT column_name concept.
     */
    template <concepts::StreamRenderable COMPONENT>
    struct Distinct : Selector<"DISTINCT", COMPONENT> {
        /**
         * Special case of DISTINCT where the uniquity column differs from the column returned.
         */
        template <concepts::StreamRenderable CRITERIA>
        struct On {
            using value_type = typename COMPONENT::value_type;

            template <size_t I>
            static auto render_to(std::ostream& stream, std::integral_constant<size_t, I> p) {
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
    template <utility::Literal TOKEN, concepts::StreamRenderable COMPONENT>
    struct Alias {
        using value_type = typename COMPONENT::value_type;

        template <size_t I>
        static auto render_to(std::ostream& stream, std::integral_constant<size_t, I> p) {
            auto result = COMPONENT::render_to(stream, p);
            stream << " AS " << TOKEN.Value;
            return result;
        }

        /**
         * A reference to the alias.
         */
        struct Reference {
            template <size_t I>
            static auto render_to(std::ostream& stream, std::integral_constant<size_t, I> p) {
                stream << TOKEN.Value;
                return p;
            }
        };
    };
}

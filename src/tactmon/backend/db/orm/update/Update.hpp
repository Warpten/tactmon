#pragma once

#include "backend/db/orm/concepts/Concepts.hpp"
#include "backend/db/orm/VariadicRenderable.hpp"

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
            return VariadicRenderable<", ", ASSIGNMENTS...>::render_to(ss, p);
        }
    };

    namespace concepts {
        namespace detail {
            template <typename T> struct IsSet : std::false_type { };
            template <typename... ASSIGNMENTS...> struct IsSet<Set<ASSIGNMENTS...>> : std::true_type { };
        }

        template <typename T>
        concept IsSet = detail::IsSet<T>::value;
    }

    template <concepts::StreamRenderable ENTITY, concepts::IsSet SET>
    struct Query final {
        static std::string render();

        template <size_t I>
        static auto render_to(std::ostream& ss, std::integral_constant<size_t, I>);

        template <concepts::StreamRenderable CRITERIA>
        struct Where final {
            static std::string render();

            template <size_t I>
            static auto render_to(std::ostream& ss, std::integral_constant<size_t, I>);
        };
    };

    template <concepts::StreamRenderable ENTITY, concepts::IsSet SET>
    template <concepts::StreamRenderable CRITERIA>
    /* static */ std::string Query<ENTITY, SET>::Where<CRITERIA>::render() {
        std::stringstream ss;
        auto queryOffset = Query<ENTITY, SET>::render_to(ss, std::integral_constant<size_t, 1> { });
        ss << " WHERE ";
        return CRITERIA::render_to(ss, queryOffset);
    }

    template <concepts::StreamRenderable ENTITY, concepts::IsSet SET>
    template <size_t I>
    /* static */ auto Query<ENTITY, SET>::render_to(std::ostream& ss, std::integral_constant<size_t, I> p) {
        ss << "UPDATE ";
        auto entityOffset = ENTITY::render_to(ss, p);
        return SET::render_to(ss, entityOffset);
    }
}

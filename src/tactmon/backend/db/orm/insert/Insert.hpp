#pragma once

#include "backend/db/orm/Concepts.hpp"
#include "backend/db/orm/Selectors.hpp"
#include "backend/db/orm/detail/VariadicRenderable.hpp"
#include "utility/Literal.hpp"

#include <ostream>
#include <sstream>
#include <string>
#include <utility>

namespace backend::db::orm::insert {
    template <utility::Literal NAME>
    struct OnConstraint {
        template <size_t I>
        static auto render_to(std::ostream& ss, std::integral_constant<size_t, I> p) {
            ss << "ON CONSTRAINT " << NAME.Value;
            return p;
        }
    };

    using Excluded = orm::Alias<"EXCLUDED">;

    /**
     * An INSERT query.
     *
     * @tparam ENTITY The entity for which a record will be inserted.
     * @tparam COLUMNS... The columns that need to be assigned.
     */
    template <concepts::StreamRenderable ENTITY, concepts::StreamRenderable... COLUMNS>
    class Query final {
        template <size_t I>
        static auto render_to(std::ostream& ss, std::integral_constant<size_t, I>);

    public:
        static std::string render();

        /**
         * An INSERT query with an ON CONFLICT clause.
         *
         * @tparam COMPONENT A conflict handling clause. See @ref UpdateFromExcluded and @ref DoNothing.
         */
        template <typename COMPONENT>
        struct OnConflict final {
            static std::string render();

            template <size_t I>
            static auto render_to(std::ostream& ss, std::integral_constant<size_t, I> p);
        };
    };

    /**
     * A conflict handling clause that updates an existing record with the set of provided values.
     *
     * @tparam ENTITY        The entity being updated.
     * @tparam PK            A column of @p ENTITY that will serve as the identifier for the UPDATE clause.
     * @tparam COMPONENTS... The components to update.
     */
    template <concepts::StreamRenderable ENTITY, concepts::StreamRenderable PK, concepts::StreamRenderable... COMPONENTS>
    struct UpdateFromExcluded final {
        // Generates a sensible ON CONFLICT DO UPDATE query that fixes the conflicting record to the given parameters.
        template <size_t P>
        static auto render_to(std::ostream& ss, std::integral_constant<size_t, P> p) {
            ss << "UPDATE SET ";
            auto setOffset = detail::VariadicRenderable<", ", Equals<COMPONENTS, typename COMPONENTS::Bind<Excluded>>...>::render_to(ss, p);
            ss << " WHERE ";
            return Equals<PK, typename PK::Bind<Excluded>>::render_to(ss, setOffset);
        }
    };

    /**
     * A conflict handling clause that does effectively nothing except silence the error.
     */
    struct DoNothing final {
        template <size_t I>
        static auto render_to(std::ostream& ss, std::integral_constant<size_t, I>) {
            ss << "NOTHING";
        }
    };

    /**
     * Default implementation of an INSERT INTO ... (...) VALUES (...) ON CONFLICT DO UPDATE SET ... WHERE ...
     * which effectively acts as MySQL's REPLACE INTO extension.
     *
     * @tparam ENTITY     The entity to be inserted or updated.
     * @tparam COLUMNS... The columns being set.
     */
    template <concepts::StreamRenderable ENTITY, concepts::StreamRenderable... COLUMNS>
    using Replace = Query<ENTITY, COLUMNS...>::OnConflict<UpdateFromExcluded<ENTITY, typename ENTITY::primary_key, COLUMNS...>>;

    namespace detail {
        template <size_t I, typename C>
        auto render_conflict_clause(std::ostream& ss, std::integral_constant<size_t, I> p, C) {
            ss << '(';
            auto result = C::render_to(ss, p);
            ss << ')';
            return result;
        }

        template <size_t I, utility::Literal N>
        auto render_conflict_clause(std::ostream& ss, std::integral_constant<size_t, I> p, OnConstraint<N>) {
            return OnConstraint<N>::render_to(ss, p);
        }
    }

    template <concepts::StreamRenderable ENTITY, concepts::StreamRenderable... COLUMNS>
    template <typename COMPONENT>
    /* static */ std::string Query<ENTITY, COLUMNS...>::OnConflict<COMPONENT>::render_to() {
        std::stringstream ss;
        render_to(ss, std::integral_constant<size_t, 1> { });
        return ss.str();
    }

    template <concepts::StreamRenderable ENTITY, concepts::StreamRenderable... COLUMNS>
    template <concepts::StreamRenderable COMPONENT>
    template <size_t I>
    static auto Query<ENTITY, COLUMNS...>::OnConflict<COMPONENT>::render_to(std::ostream& ss, std::integral_constant<size_t, I> p) {
        auto queryOffset = Query<ENTITY>::render_to(ss, p);
        ss << " ON CONFLICT DO";
        return detail::render_conflict_clause(ss, queryOffset, COMPONENT{ });
    }

    template <concepts::StreamRenderable ENTITY, concepts::StreamRenderable... COLUMNS>
    /* static */ std::string Query<ENTITY, COLUMNS...>::render() {
        std::stringstream ss;
        render_to(ss, std::integral_constant<size_t, 1> { });
        return ss.str();
    }

    template <concepts::StreamRenderable ENTITY, concepts::StreamRenderable... COLUMNS>
    template <size_t I>
    /* static */ auto Query<ENTITY. COLUMNS...>::render_to(std::ostream& ss, std::integral_constant<size_t, I>) {
        ss << "INSERT INTO ";
        auto projectionOffset = ENTITY::render_to(ss, std::integral_constant<size_t, 1> { });
        ss << " (";
        auto componentsOffset = detail::VariadicRenderable<", ", COLUMNS...>::render_to(ss, projectionOffset);
        ss << ") VALUES (";
        // TODO: $1, $2, ..., $N
        ss << ")";
    }
}

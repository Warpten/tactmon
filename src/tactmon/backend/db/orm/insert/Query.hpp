#pragma once

#include "backend/db/orm/Concepts.hpp"
#include "backend/db/orm/Predicates.hpp"
#include "backend/db/orm/Query.hpp"
#include "backend/db/orm/Selectors.hpp"
#include "backend/db/orm/detail/VariadicRenderable.hpp"
#include "utility/Literal.hpp"
#include "utility/Tuple.hpp"

#include <ostream>
#include <sstream>
#include <string>
#include <utility>

#include <pqxx/transaction>

namespace backend::db::insert {
    template <utility::Literal NAME>
    struct OnConstraint {
        using parameter_types = utility::tuple<>;

        template <std::size_t I>
        static auto render_to(std::ostream& ss, std::integral_constant<std::size_t, I> p) {
            ss << "ON CONSTRAINT " << NAME.Value;
            return p;
        }
    };

    using Excluded = db::Raw<utility::Literal { "EXCLUDED" }>;

    /**
     * An INSERT query.
     *
     * @tparam ENTITY The entity for which a record will be inserted.
     * @tparam COLUMNS... The columns that need to be assigned.
     */
    template <typename ENTITY, typename... COLUMNS>
    class Query final : public IQuery<Query<ENTITY, COLUMNS...>> {
        friend struct IQuery<Query<ENTITY, COLUMNS...>>;

        template <std::size_t I>
        static auto render_to(std::ostream& ss, std::integral_constant<std::size_t, I> p) {
            ss << "INSERT INTO ";
            auto projectionOffset = ENTITY::render_to(ss, p);
            ss << " (";
            auto componentsOffset = detail::VariadicRenderable<", ", COLUMNS...>::render_to(ss, projectionOffset);
            ss << ") VALUES (";

            // TODO: There are cases where values inserted will be derived from joins
            //       This needs to be fixed to handle such values as it'll break otherwise.
            for (size_t i = 0; i < sizeof...(COLUMNS); ++i)
                ss << '$' << (componentsOffset.value + i);
            ss << ")";
            return std::integral_constant<std::size_t, componentsOffset.value + sizeof...(COLUMNS)> { };
        }

    public:
        using parameter_types = decltype(utility::tuple_cat(
            std::declval<typename ENTITY::parameter_types>(),
            std::declval<utility::tuple<typename COLUMNS::value_type>>()...
        ));
        using transaction_type = pqxx::transaction<pqxx::isolation_level::read_committed, pqxx::write_policy::read_write>;
        using result_type = ENTITY;

        template <typename PROJECTION>
        friend struct Returning;

        template <typename COMPONENT>
        friend struct OnConflict;

        template <typename PROJECTION>
        class Returning : public IQuery<Returning<PROJECTION>> {
            friend struct IQuery<Returning<PROJECTION>>;

            template <std::size_t I>
            static auto render_to(std::ostream& ss, std::integral_constant<std::size_t, I> p) {
                auto queryOffset = Query<ENTITY, COLUMNS...>::render_to(ss, p);
                ss << " RETURNING ";
                return PROJECTION::render_to(ss, queryOffset);
            }

        public:
            using parameter_types = decltype(utility::tuple_cat(
                std::declval<utility::tuple<typename COLUMNS::value_type>>()...,
                std::declval<typename PROJECTION::parameter_types>()
            ));
            using transaction_type = pqxx::transaction<pqxx::isolation_level::read_committed, pqxx::write_policy::read_write>;
            using result_type = ENTITY;
        };

        /**
         * An INSERT query with an ON CONFLICT clause.
         *
         * @tparam COMPONENT A conflict handling clause. See UpdateFromExcluded and DoNothing.
         */
        template <typename COMPONENT>
        class OnConflict final : public IQuery<OnConflict<COMPONENT>> {
            friend struct IQuery<OnConflict<COMPONENT>>;

            template <std::size_t I>
            static auto render_to(std::ostream& ss, std::integral_constant<std::size_t, I> p);

        public:
            using parameter_types = decltype(utility::tuple_cat(
                std::declval<utility::tuple<typename COLUMNS::value_type>>()...,
                std::declval<typename COMPONENT::parameter_types>()
            ));
            using transaction_type = pqxx::transaction<pqxx::isolation_level::read_committed, pqxx::write_policy::read_write>;
            using result_type = ENTITY;

            template <typename PROJECTION>
            class Returning final : public IQuery<Returning<PROJECTION>> {
                template <std::size_t P>
                static auto render_to(std::ostream& ss, std::integral_constant<std::size_t, P> p) {
                    auto baseOffset = OnConflict<COMPONENT>::render_to(ss, p);
                    ss << " RETURNING ";
                    return COMPONENT::render_to(ss, baseOffset);
                }

            public:
                using parameter_types = decltype(utility::tuple_cat(
                    std::declval<utility::tuple<typename COLUMNS::value_type>>()...,
                    std::declval<typename COMPONENT::parameter_types>(),
                    std::declval<typename PROJECTION::parameter_types>()
                ));

                using transaction_type = pqxx::transaction<pqxx::isolation_level::read_committed, pqxx::write_policy::read_write>;
                using result_type = ENTITY;
            };
        };
    };

    /**
     * A conflict handling clause that updates an existing record with the set of provided values.
     *
     * @tparam ENTITY        The entity being updated.
     * @tparam PK            A column of @p ENTITY that will serve as the identifier for the UPDATE clause.
     * @tparam COMPONENTS... The components to update.
     */
    template <typename ENTITY, typename PK, typename... COMPONENTS>
    struct UpdateFromExcluded final {
        using parameter_types = decltype(utility::tuple_cat(
            std::declval<typename ENTITY::parameter_types>(),
            std::declval<typename PK::parameter_types>(),
            std::declval<typename COMPONENTS::parameter_types>()...
        ));

        // Generates a sensible ON CONFLICT DO UPDATE query that fixes the conflicting record to the given parameters.
        template <std::size_t P>
        static auto render_to(std::ostream& ss, std::integral_constant<std::size_t, P> p) {
            ss << "UPDATE SET ";
            auto setOffset = detail::VariadicRenderable<
                ", ",
                Equals<
                    COMPONENTS,
                    typename COMPONENTS::template BindToProjection<Excluded>
                >...
            >::render_to(ss, p);
            ss << " WHERE ";
            return Equals<PK, typename PK::template BindToProjection<Excluded>>::render_to(ss, setOffset);
        }
    };

    /**
     * A conflict handling clause that does effectively nothing except silence the error.
     */
    struct DoNothing final {
        using parameter_types = utility::tuple<>;

        template <std::size_t I>
        static auto render_to(std::ostream& ss, std::integral_constant<std::size_t, I>) {
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
    template <typename ENTITY, typename... COLUMNS>
    using Replace = Query<ENTITY, COLUMNS...>::OnConflict<UpdateFromExcluded<ENTITY, typename ENTITY::primary_key, COLUMNS...>>;

    namespace detail {
        template <std::size_t I, typename C>
        auto render_conflict_clause(std::ostream& ss, std::integral_constant<std::size_t, I> p, C) {
            ss << '(';
            auto result = C::render_to(ss, p);
            ss << ')';
            return result;
        }

        template <std::size_t I, utility::Literal N>
        auto render_conflict_clause(std::ostream& ss, std::integral_constant<std::size_t, I> p, OnConstraint<N>) {
            return OnConstraint<N>::render_to(ss, p);
        }
    }

    template <typename ENTITY, typename... COLUMNS>
    template <typename COMPONENT>
    template <std::size_t I>
    static auto Query<ENTITY, COLUMNS...>::OnConflict<COMPONENT>::render_to(std::ostream& ss, std::integral_constant<std::size_t, I> p) {
        auto queryOffset = Query<ENTITY, COLUMNS...>::render_to(ss, p);
        ss << " ON CONFLICT DO";
        return detail::render_conflict_clause(ss, queryOffset, COMPONENT{ });
    }
}

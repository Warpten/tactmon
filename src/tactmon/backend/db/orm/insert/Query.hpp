#pragma once

#include "backend/db/orm/Concepts.hpp"
#include "backend/db/orm/Predicates.hpp"
#include "backend/db/orm/Query.hpp"
#include "backend/db/orm/Selectors.hpp"
#include "backend/db/orm/Shared.hpp"
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

        template <std::size_t PARAMETER>
        constexpr static auto render_to_v2(std::string prev, std::integral_constant<std::size_t, PARAMETER> p) {
            return std::make_pair(prev + "ON CONSTRAINT " + NAME.Value, p);
        }
    };

    using Excluded = db::Raw<utility::Literal { "EXCLUDED" }>;

    template <typename COLUMN, typename VALUE = Parameter>
    struct Value final {
        using column_type = COLUMN;
        using parameter_type = typename VALUE::template BindToProjection<COLUMN>;

        using parameter_types = typename parameter_type::parameter_types;
    };

    namespace detail {
        template <std::size_t I, typename C>
        constexpr auto render_conflict_clause_v2(std::string prev, std::integral_constant<std::size_t, I> p, C) {
            return C::render_to_v2(prev, p);
        }

        template <std::size_t I, utility::Literal N>
        constexpr auto render_conflict_clause_v2(std::string prev, std::integral_constant<std::size_t, I> p, OnConstraint<N>) {
            return OnConstraint<N>::render_to_v2(prev, p);
        }

        template <typename ENTITY, typename... COLUMNS>
        struct QueryImpl {
            template <std::size_t PARAMETER>
            constexpr static auto render_to_v2(std::string prev, std::integral_constant<std::size_t, PARAMETER> p) {
                auto [next, u] = ENTITY::render_to_v2(prev + "INSERT INTO ", p);
                auto [next2, u2] = db::detail::VariadicRenderable<", ", typename COLUMNS::column_type...>::render_to_v2(next + " (", u);
                auto [next3, u3] = db::detail::VariadicRenderable<", ", typename COLUMNS::parameter_type...>::render_to_v2(next2 + ") VALUES (", u2);
                return std::make_pair(next3 + ')', u3);
            }

            using parameter_types = decltype(utility::tuple_cat(
                std::declval<typename ENTITY::parameter_types>(),
                std::declval<typename COLUMNS::parameter_types>()...
            ));
            using transaction_type = pqxx::transaction<pqxx::isolation_level::read_committed, pqxx::write_policy::read_write>;
            using result_type = ENTITY;
        };

        template <typename QUERYBASE, typename CONFLICTING, typename COMPONENT>
        struct OnConflictImpl final {
            template <std::size_t PARAMETER>
            constexpr static auto render_to_v2(std::string prev, std::integral_constant<std::size_t, PARAMETER> p) {
                auto [next, u] = QUERYBASE::render_to_v2(prev, p);
                auto [next2, u2] = CONFLICTING::render_to_v2(next + " ON CONFLICT (", u);
                return detail::render_conflict_clause_v2(next2 + ") DO ", u2, COMPONENT{ });
            }

            using parameter_types = decltype(utility::tuple_cat(
                std::declval<typename QUERYBASE::parameter_types>(),
                std::declval<typename COMPONENT::parameter_types>()
            ));
            using transaction_type = pqxx::transaction<pqxx::isolation_level::read_committed, pqxx::write_policy::read_write>;
            using result_type = typename QUERYBASE::result_type;
        };

        template <typename QUERYBASE, typename COMPONENT>
        struct ReturningImpl final {
            template <std::size_t PARAMETER>
            constexpr static auto render_to_v2(std::string prev, std::integral_constant<std::size_t, PARAMETER> p) {
                auto [next, u] = QUERYBASE::render_to_v2(prev, p);
                return COMPONENT::render_to_v2(next + " RETURNING ", u);
            }

            using parameter_types = decltype(utility::tuple_cat(
                std::declval<typename QUERYBASE::parameter_types>(),
                std::declval<typename COMPONENT::parameter_types>()
            ));

            using transaction_type = pqxx::transaction<pqxx::isolation_level::read_committed, pqxx::write_policy::read_write>;
            using result_type = typename QUERYBASE::result_type;
        };
    }

    /**
     * An INSERT query.
     *
     * @tparam ENTITY The entity for which a record will be inserted.
     * @tparam COLUMNS... The columns that need to be assigned.
     */
    template <typename ENTITY, typename... COLUMNS>
    struct Query final : IQuery<detail::QueryImpl<ENTITY, COLUMNS...>> {
        /**
         * An INSERT query with an ON CONFLICT clause.
         *
         * @tparam COMPONENT A conflict handling clause. See UpdateFromExcluded and DoNothing.
         */
        template <typename PROJECTION>
        struct Returning final : IQuery<
            detail::ReturningImpl<
                detail::QueryImpl<ENTITY, COLUMNS...>,
                PROJECTION
            >
        > { };

        /**
         * A SQL INSERT query with a conflict handling clause.
         *
         * @param[in] CONFLICTING A component identifying the column whose value is conflicting with another record.
         * @param[in] COMPONENT   A component describing how the conflict is handled.
         */
        template <typename CONFLICTING, typename COMPONENT>
        struct OnConflict final : IQuery<
            detail::OnConflictImpl<
                detail::QueryImpl<ENTITY, COLUMNS...>,
                CONFLICTING,
                COMPONENT
            >
        > {
            template <typename PROJECTION>
            struct Returning final : IQuery<
                detail::ReturningImpl<
                    detail::OnConflictImpl<
                        detail::QueryImpl<ENTITY, COLUMNS...>,
                        CONFLICTING,
                        COMPONENT
                    >, PROJECTION
                >
            > { };
        };
    };

    /**
     * A conflict handling clause that updates an existing record with the set of provided values.
     *
     * @tparam ENTITY        The entity being updated.
     * @tparam PK            A column of @p ENTITY that will serve as the identifier for the UPDATE clause.
     * @tparam COMPONENTS... The components to update.
     */
    template <typename ENTITY, typename... COMPONENTS>
    struct UpdateFromExcluded final {
        using parameter_types = decltype(utility::tuple_cat(
            std::declval<typename ENTITY::parameter_types>(),
            std::declval<typename COMPONENTS::parameter_types>()...
        ));

        template <std::size_t PARAMETER>
        constexpr static auto render_to_v2(std::string prev, std::integral_constant<std::size_t, PARAMETER> p) {
            return db::detail::VariadicRenderable<", ", Equals<
                COMPONENTS,
                typename COMPONENTS::template BindToProjection<Excluded>
            >...>::render_to_v2(prev + "UPDATE SET ", p);
        }
    };

    /**
     * A conflict handling clause that does effectively nothing except silence the error.
     */
    struct DoNothing final {
        using parameter_types = utility::tuple<>;

        template <std::size_t PARAMETER>
        constexpr static auto render_to_v2(std::string prev, std::integral_constant<std::size_t, PARAMETER> p) {
            return std::make_pair(prev + "NOTHING", p);
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
    using Replace = typename Query<ENTITY, COLUMNS...>::template OnConflict<UpdateFromExcluded<ENTITY, typename COLUMNS::column_type...>>;

}

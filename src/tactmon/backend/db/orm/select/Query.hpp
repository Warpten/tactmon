#pragma once

#include "backend/db/orm/Concepts.hpp"
#include "backend/db/orm/Query.hpp"
#include "backend/db/orm/select/Concepts.hpp"
#include "backend/db/orm/detail/VariadicRenderable.hpp"
#include "utility/Literal.hpp"
#include "utility/Tuple.hpp"

#include <sstream>
#include <string>
#include <utility>

#include <pqxx/transaction>

namespace backend::db::select {
    namespace concepts {
        using namespace db::concepts;
    }

    namespace detail {
        template <typename R, typename P, typename E, typename... CS>
        struct QueryImpl {
            template <std::size_t I>
            constexpr static auto render_to_v2(std::string prev, std::integral_constant<std::size_t, I> p) {
                auto [next, u] = P::render_to_v2(prev + "SELECT ", p);
                auto [next2, u2] = E::render_to_v2(next + " FROM ", u);
                if constexpr (sizeof...(CS) > 0)
                    return db::detail::VariadicRenderable<" ", CS...>::render_to_v2(next2 + ' ', u2);
                else
                    return db::detail::VariadicRenderable<" ", CS...>::render_to_v2(next2, u2);
            }

            using parameter_types = decltype(utility::tuple_cat(
                std::declval<typename P::parameter_types>(),
                std::declval<typename E::parameter_types>(),
                std::declval<typename CS::parameter_types>()...
            ));
            using transaction_type = pqxx::transaction<pqxx::isolation_level::read_committed, pqxx::write_policy::read_only>;
            using result_type = R;
        };

        /**
         * Introduces CTEs into a given query.
         *
         * @tparam ES... A sequence of specializations of @p CTE.
         *
         * @remarks This implementation is a bit backwards; if your CTEs include positional parameters from the main query, they need to be
         *          declared in the CTE first. That is, parameters in the main query should be specializations of @p BoundParameter.
         */
        template <typename QUERYBASE, concepts::IsCTE... ES>
        struct WithImpl final {
            template <std::size_t I>
            constexpr static auto render_to_v2(std::string prev, std::integral_constant<std::size_t, I> p) {
                auto [next, u] = db::detail::VariadicRenderable<", ", ES...>::render_to_v2(prev + "WITH ", p);
                return QUERYBASE::render_to_v2(next + ' ', u);
            }

        public:
            using parameter_types = decltype(utility::tuple_cat(
                std::declval<typename ES::parameter_types>()...,
                std::declval<typename QUERYBASE::parameter_types>()
            ));
        };
    }

    /**
     * A SQL Select query.
     *
     * @tparam PROJECTION    A projection corresponding to the values returned by this query.
     * @tparam ENTITY        The main entity being queried.
     * @tparam COMPONENTS... Extraneous components used to generate the query.
     */
    template <typename PROJECTION, typename ENTITY, typename... COMPONENTS>
    struct Query final : IQuery<detail::QueryImpl<PROJECTION, PROJECTION, ENTITY, COMPONENTS...>> {
        template <concepts::IsCTE... EXPRESSIONS>
        struct With final : IQuery<
            detail::WithImpl<
                detail::QueryImpl<PROJECTION, PROJECTION, ENTITY, COMPONENTS...>,
                EXPRESSIONS...
            >
        > { };
    };

    template <typename ENTITY, typename... COMPONENTS>
    struct Query<ENTITY, ENTITY, COMPONENTS...> final : IQuery<detail::QueryImpl<ENTITY, typename ENTITY::projection_type, ENTITY, COMPONENTS...>> {
        template <concepts::IsCTE... EXPRESSIONS>
        struct With final : IQuery<
            detail::WithImpl<
                detail::QueryImpl<ENTITY, typename ENTITY::projection_type, ENTITY, COMPONENTS...>,
                EXPRESSIONS...
            >
        > { };
    };
}

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
        class QueryImpl : public IQuery<QueryImpl<R, P, E, CS...>> {
            friend struct IQuery<QueryImpl<R, P, E, CS...>>;

            template <size_t I>
            static auto render_to(std::ostream& ss, std::integral_constant<size_t, I>);

        public:
            using parameter_types = decltype(utility::tuple_cat(
                std::declval<typename P::parameter_types>(),
                std::declval<typename E::parameter_types>(),
                std::declval<typename CS::parameter_types>()...
            ));
            using transaction_type = pqxx::transaction<pqxx::isolation_level::read_committed, pqxx::write_policy::read_only>;
            using result_type = R;

            /**
             * Introduces CTEs into a given query.
             *
             * @tparam ES... A sequence of specializations of @p CTE.
             *
             * @remarks This implementation is a bit backwards; if your CTEs include positional parameters from the main query, they need to be
             *          declared in the CTE first. That is, parameters in the main query should be specializations of @p BoundParameter.
             */
            template <concepts::IsCTE... ES>
            class With final : public IQuery<With<ES...>> {
                friend struct IQuery<With<ES...>>;

                template <size_t I>
                static auto render_to(std::ostream& ss, std::integral_constant<size_t, I>);

            public:
                using parameter_types = decltype(utility::tuple_cat(
                    std::declval<typename P::parameter_types>(),
                    std::declval<typename E::parameter_types>(),
                    std::declval<typename CS::parameter_types>()...,
                    std::declval<typename ES::parameter_types>()...
                ));
            };
        };

        template <typename R, typename P, typename E, typename... CS>
        template <size_t I>
        /* static */ auto QueryImpl<R, P, E, CS...>::render_to(std::ostream& ss, std::integral_constant<size_t, I>) {
            ss << "SELECT "; auto projectionOffset = P::render_to(ss, std::integral_constant<size_t, 1> { });
            ss << " FROM ";  auto entityOffset = E::render_to(ss, projectionOffset);

            if constexpr (sizeof...(CS) > 0)
                ss << ' ';
            return db::detail::VariadicRenderable<" ", CS...>::render_to(ss, entityOffset);
        }

        template <typename R, typename P, typename E, typename... CS>
        template <concepts::IsCTE... ES>
        template <size_t I>
        /* static */ auto QueryImpl<R, P, E, CS...>::With<ES...>::render_to(std::ostream& ss, std::integral_constant<size_t, I> p) {
            ss << "WITH ";
            auto cteOffset = db::detail::VariadicRenderable<", ", ES...>::render_to(ss, p);
            ss << ' ';
            return QueryImpl<R, P, E, CS...>::render_to(ss, cteOffset);
        }
    }

    /**
     * A SQL Select query.
     *
     * @tparam PROJECTION    A projection corresponding to the values returned by this query.
     * @tparam ENTITY        The main entity being queried.
     * @tparam COMPONENTS... Extraneous components used to generate the query.
     */
    template <typename PROJECTION, typename ENTITY, typename... COMPONENTS>
    class Query final : detail::QueryImpl<PROJECTION, PROJECTION, ENTITY, COMPONENTS...> {
        using Base = detail::QueryImpl<PROJECTION, PROJECTION, ENTITY, COMPONENTS...>;

    public:
        static std::string render() { return Base::render(); }

        template <concepts::IsCTE... EXPRESSIONS>
        using With = typename Base::template With<EXPRESSIONS...>;

        using parameter_types = typename Base::parameter_types;
        using transaction_type = typename Base::transaction_type;
        using result_type = typename Base::result_type;
    };

    template <typename ENTITY, typename... COMPONENTS>
    class Query<ENTITY, ENTITY, COMPONENTS...> final : detail::QueryImpl<ENTITY, typename ENTITY::projection_type, ENTITY, COMPONENTS...> {
        using Base = detail::QueryImpl<ENTITY, typename ENTITY::projection_type, ENTITY, COMPONENTS...>;

    public:
        static std::string render() { return Base::render(); }

        template <concepts::IsCTE... EXPRESSIONS>
        using With = typename Base::template With<EXPRESSIONS...>;

        using parameter_types = typename Base::parameter_types;
        using transaction_type = typename Base::transaction_type;
        using result_type = typename Base::result_type;
    };
}

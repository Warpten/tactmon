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

    /**
     * A SQL Select query.
     *
     * @tparam PROJECTION    A projection corresponding to the values returned by this query.
     * @tparam ENTITY        The main entity being queried.
     * @tparam COMPONENTS... Extraneous components used to generate the query.
     */
    template <typename PROJECTION, typename ENTITY, typename... COMPONENTS>
    class Query final : public IQuery<Query<PROJECTION, ENTITY, COMPONENTS...>> {
        friend struct IQuery<Query<PROJECTION, ENTITY, COMPONENTS...>>;

        template <size_t I>
        static auto render_to(std::ostream& ss, std::integral_constant<size_t, I>);

    public:
        using parameter_types = decltype(utility::tuple_cat(
                std::declval<typename PROJECTION::parameter_types>(),
                std::declval<typename ENTITY::parameter_types>(),
                std::declval<typename COMPONENTS::parameter_types>()...
        ));
        using transaction_type = pqxx::transaction<pqxx::isolation_level::read_committed, pqxx::write_policy::read_only>;
        using result_type = PROJECTION;

        /**
         * Introduces CTEs into a given query.
         *
         * @tparam EXPRESSIONS... A sequence of specializations of @p CTE.
         *
         * @remarks This implementation is a bit backwards; if your CTEs include positional parameters from the main query, they need to be
         *          declared in the CTE first. That is, parameters in the main query should be specializations of @p BoundParameter.
         */
        template <concepts::IsCTE... EXPRESSIONS>
        class With final : public IQuery<With<EXPRESSIONS...>> {
            friend struct IQuery<With<EXPRESSIONS...>>;

            template <size_t I>
            static auto render_to(std::ostream& ss, std::integral_constant<size_t, I>);

        public:
            using parameter_types = decltype(utility::tuple_cat(
                std::declval<typename PROJECTION::parameter_types>(),
                std::declval<typename ENTITY::parameter_types>(),
                std::declval<typename COMPONENTS::parameter_types>()...,
                std::declval<typename EXPRESSIONS::parameter_types>()...
            ));
        };
    };

    template <typename PROJECTION, typename ENTITY, typename... COMPONENTS>
    template <size_t I>
    /* static */ auto Query<PROJECTION, ENTITY, COMPONENTS...>::render_to(std::ostream& ss, std::integral_constant<size_t, I>) {
        ss << "SELECT "; auto projectionOffset = PROJECTION::render_to(ss, std::integral_constant<size_t, 1> { });
        ss << " FROM ";  auto entityOffset = ENTITY::render_to(ss, projectionOffset);

        if constexpr (sizeof...(COMPONENTS) > 0)
            ss << ' ';
        return detail::VariadicRenderable<" ", COMPONENTS...>::render_to(ss, entityOffset);
    }

    template <typename PROJECTION, typename ENTITY, typename... COMPONENTS>
    template <concepts::IsCTE... EXPRESSIONS>
    template <size_t I>
    /* static */ auto Query<PROJECTION, ENTITY, COMPONENTS...>::With<EXPRESSIONS...>::render_to(std::ostream& ss, std::integral_constant<size_t, I> p) {
        ss << "WITH ";
        auto cteOffset = detail::VariadicRenderable<", ", EXPRESSIONS...>::render_to(ss, p);
        ss << ' ';
        return Query<PROJECTION, ENTITY, COMPONENTS...>::render_to(ss, cteOffset);
    }
}

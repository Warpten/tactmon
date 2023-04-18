#pragma once

#include "backend/db/orm/Concepts.hpp"
#include "backend/db/orm/Query.hpp"
#include "backend/db/orm/detail/VariadicRenderable.hpp"
#include "backend/db/orm/update/Set.hpp"
#include "backend/db/orm/update/Concepts.hpp"

#include <pqxx/transaction>

namespace backend::db::update {
    namespace concepts {
        using namespace db::concepts;
    }

    namespace detail {
        // Basic implementation of an UPDATE query.
        template <typename ENTITY, concepts::IsSet SET>
        struct QueryImpl {
            template <std::size_t I>
            static auto render_to(std::ostream& ss, std::integral_constant<std::size_t, I>);

            template <std::size_t I>
            constexpr static auto render_to_v2(std::string prev, std::integral_constant<std::size_t, I> p) {
                auto [next, u] = ENTITY::render_to_v2(prev + "UPDATE ", p);
                return SET::render_to_v2(next + ' ', u);
            }

            using parameter_types = decltype(utility::tuple_cat(
                std::declval<typename ENTITY::parameter_types>(),
                std::declval<typename SET::parameter_types>()
            ));
            using transaction_type = pqxx::transaction<pqxx::isolation_level::read_committed, pqxx::write_policy::read_write>;
            using result_type = void;
        };

        // Basic implementation of a WHERE clause on top of an UPDATE query.
        template <typename QUERYBASE, typename CRITERIA>
        struct WhereImpl final {
            template <std::size_t I>
            static auto render_to(std::ostream& ss, std::integral_constant<std::size_t, I> p) {
                auto queryOffset = QUERYBASE::render_to(ss, std::integral_constant<std::size_t, 1> { });
                ss << " WHERE ";
                return CRITERIA::render_to(ss, queryOffset);
            }

            template <std::size_t I>
            constexpr static auto render_to_v2(std::string prev, std::integral_constant<std::size_t, I> p) {
                auto [next, u] = QUERYBASE::render_to_v2(prev, p);
                return CRITERIA::render_to_v2(next + " WHERE ", u);
            }

        public:
            using parameter_types = decltype(utility::tuple_cat(
                std::declval<typename QUERYBASE::parameter_types>(),
                std::declval<typename CRITERIA::parameter_types>()
            ));
            using transaction_type = pqxx::transaction<pqxx::isolation_level::read_committed, pqxx::write_policy::read_write>;
            using result_type = void;
        };

        template <typename ENTITY, concepts::IsSet SET>
        template <std::size_t I>
        /* static */ auto QueryImpl<ENTITY, SET>::render_to(std::ostream& ss, std::integral_constant<std::size_t, I> p) {
            ss << "UPDATE ";
            auto entityOffset = ENTITY::render_to(ss, p);
            ss << ' ';
            return SET::render_to(ss, entityOffset);
        }
    }

    template <typename ENTITY, concepts::IsSet SET>
    struct Query final : IQuery<detail::QueryImpl<ENTITY, SET>> {
        template <typename CRITERIA>
        struct Where final : IQuery<
            detail::WhereImpl<
                detail::QueryImpl<ENTITY, SET>,
                CRITERIA
            >
        > { };
    };
}

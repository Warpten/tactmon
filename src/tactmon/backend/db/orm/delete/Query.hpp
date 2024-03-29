#pragma once

#include "backend/db/orm/Query.hpp"
#include "backend/db/orm/Concepts.hpp"

#include <cstdint>
#include <sstream>
#include <utility>

#include <pqxx/transaction>

namespace backend::db::del {
    namespace detail {
        template <typename ENTITY, typename CRITERIA>
        struct QueryImpl final {
            using parameter_types = typename CRITERIA::parameter_types;
            using transaction_type = pqxx::transaction<pqxx::isolation_level::read_committed, pqxx::write_policy::read_write>;
            using result_type = void;

            template <std::size_t PARAMETER>
            constexpr static auto render_to(std::string prev, std::integral_constant<std::size_t, PARAMETER> p) {
                auto [next, u] = ENTITY::render_to(prev + "DELETE FROM", p);
                return CRITERIA::render_to(next + ' ', u);
            }
        };
    }

    template <typename ENTITY, typename CRITERIA>
    struct Query final : IQuery<detail::QueryImpl<ENTITY, CRITERIA>> {

    };
}

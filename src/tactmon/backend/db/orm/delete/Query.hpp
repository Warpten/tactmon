#pragma once

#include "backend/db/orm/Query.hpp"
#include "backend/db/orm/Concepts.hpp"

#include <cstdint>
#include <sstream>
#include <utility>

#include <pqxx/transaction>

namespace backend::db::del {
    template <typename ENTITY, typename CRITERIA>
    struct Query final : IQuery<Query<ENTITY, CRITERIA>> {
        friend struct IQuery<Query<ENTITY, CRITERIA>>;

        using parameter_types = typename CRITERIA::parameter_types;
        using transaction_type = pqxx::transaction<pqxx::isolation_level::read_committed, pqxx::write_policy::read_write>;
        using result_type = void;

        template <std::size_t I>
        static auto render_to(std::ostream& ss, std::integral_constant<std::size_t, I>);

        template <std::size_t PARAMETER>
        constexpr static auto render_to_v2(std::string prev, std::integral_constant<std::size_t, PARAMETER> p) {
            auto [next, u] = ENTITY::render_to_v2(prev + "DELETE FROM", p);
            return CRITERIA::render_to_v2(next + ' ', u);
        }
    };

    template <typename ENTITY, typename CRITERIA>
    template <std::size_t I>
    auto Query<ENTITY, CRITERIA>::render_to(std::ostream& ss, std::integral_constant<std::size_t, I> p) {
        ss << "DELETE FROM ";
        auto entityOffset = ENTITY::render_to(ss, p);
        ss << ' ';
        return CRITERIA::render_to(ss, entityOffset);
    }
}

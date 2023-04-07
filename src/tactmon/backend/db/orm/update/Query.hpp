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

    template <typename ENTITY, concepts::IsSet SET>
    class Query final : public IQuery<Query<ENTITY, SET>> {
        friend struct IQuery<Query<ENTITY, SET>>;

        template <std::size_t I>
        static auto render_to(std::ostream& ss, std::integral_constant<std::size_t, I>);

    public:
        using parameter_types = typename ENTITY::parameter_types;
        using transaction_type = pqxx::transaction<pqxx::isolation_level::read_committed, pqxx::write_policy::read_write>;
        using result_type = void;

        template <typename CRITERIA>
        class Where final : public IQuery<Where<CRITERIA>> {
            friend struct IQuery<Where<CRITERIA>>;

            template <std::size_t I>
            static auto render_to(std::ostream& ss, std::integral_constant<std::size_t, I> p) {
                auto queryOffset = Query<ENTITY, SET>::render_to(ss, std::integral_constant<std::size_t, 1> { });
                ss << " WHERE ";
                return CRITERIA::render_to(ss, queryOffset);
            }

        public:
            using parameter_types = decltype(utility::tuple_cat(
                std::declval<typename ENTITY::parameter_types>(),
                std::declval<typename SET::parameter_types>(),
                std::declval<typename CRITERIA::parameter_types>()
            ));
            using transaction_type = pqxx::transaction<pqxx::isolation_level::read_committed, pqxx::write_policy::read_write>;
            using result_type = void;
        };
    };

    template <typename ENTITY, concepts::IsSet SET>
    template <std::size_t I>
    /* static */ auto Query<ENTITY, SET>::render_to(std::ostream& ss, std::integral_constant<std::size_t, I> p) {
        ss << "UPDATE ";
        auto entityOffset = ENTITY::render_to(ss, p);
        return SET::render_to(ss, entityOffset);
    }
}

#pragma once

#include "backend/db/orm/Concepts.hpp"
#include "backend/db/orm/detail/VariadicRenderable.hpp"
#include "backend/db/orm/update/Set.hpp"
#include "backend/db/orm/update/Concepts.hpp"

#include <pqxx/transaction>

namespace backend::db::update {
    namespace concepts {
        using namespace db::concepts;
    }

    template <typename ENTITY, concepts::IsSet SET>
    struct Query final {
        using parameter_types = typename ENTITY::parameter_types;
        using transaction_type = pqxx::transaction<pqxx::isolation_level::read_committed, pqxx::write_policy::read_write>;
        using result_type = void;

        static std::string render() {
            std::stringstream ss;
            render_to(ss, std::integral_constant<size_t, 1> { });
            return ss.str();
        }

        template <size_t I>
        static auto render_to(std::ostream& ss, std::integral_constant<size_t, I>);

        template <typename CRITERIA>
        struct Where final {
            using parameter_types = decltype(utility::tuple_cat(
                std::declval<typename ENTITY::parameter_types>(),
                std::declval<typename SET::parameter_types>(),
                std::declval<typename CRITERIA::parameter_types>()
            ));
            using transaction_type = pqxx::transaction<pqxx::isolation_level::read_committed, pqxx::write_policy::read_write>;
            using result_type = void;

            static std::string render();

            template <size_t I>
            static auto render_to(std::ostream& ss, std::integral_constant<size_t, I> p) {
                auto queryOffset = Query<ENTITY, SET>::render_to(ss, std::integral_constant<size_t, 1> { });
                ss << " WHERE ";
                return CRITERIA::render_to(ss, queryOffset);
            }
        };
    };

    template <typename ENTITY, concepts::IsSet SET>
    template <typename CRITERIA>
    /* static */ std::string Query<ENTITY, SET>::Where<CRITERIA>::render() {
        std::stringstream ss;
        render_to(ss, std::integral_constant<size_t, 1> { });
        return ss.str();
    }

    template <typename ENTITY, concepts::IsSet SET>
    template <size_t I>
    /* static */ auto Query<ENTITY, SET>::render_to(std::ostream& ss, std::integral_constant<size_t, I> p) {
        ss << "UPDATE ";
        auto entityOffset = ENTITY::render_to(ss, p);
        return SET::render_to(ss, entityOffset);
    }
}

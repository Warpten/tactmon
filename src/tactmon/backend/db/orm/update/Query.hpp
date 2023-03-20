#pragma once

#include "backend/db/orm/Concepts.hpp"
#include "backend/db/orm/detail/VariadicRenderable.hpp"
#include "backend/db/orm/update/Set.hpp"

namespace backend::db::orm::update {
    namespace concepts {
        using namespace orm::concepts;
    }

    template <concepts::StreamRenderable ENTITY, concepts::IsSet SET>
    struct Query final {
        static std::string render();

        template <size_t I>
        static auto render_to(std::ostream& ss, std::integral_constant<size_t, I>);

        template <concepts::StreamRenderable CRITERIA>
        struct Where final {
            static std::string render();

            template <size_t I>
            static auto render_to(std::ostream& ss, std::integral_constant<size_t, I>);
        };
    };

    template <concepts::StreamRenderable ENTITY, concepts::IsSet SET>
    template <concepts::StreamRenderable CRITERIA>
    /* static */ std::string Query<ENTITY, SET>::Where<CRITERIA>::render() {
        std::stringstream ss;
        auto queryOffset = Query<ENTITY, SET>::render_to(ss, std::integral_constant<size_t, 1> { });
        ss << " WHERE ";
        return CRITERIA::render_to(ss, queryOffset);
    }

    template <concepts::StreamRenderable ENTITY, concepts::IsSet SET>
    template <size_t I>
    /* static */ auto Query<ENTITY, SET>::render_to(std::ostream& ss, std::integral_constant<size_t, I> p) {
        ss << "UPDATE ";
        auto entityOffset = ENTITY::render_to(ss, p);
        return SET::render_to(ss, entityOffset);
    }
}

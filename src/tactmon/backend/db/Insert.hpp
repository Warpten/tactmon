#pragma once

#include "backend/db/DSL.hpp"

#include <sstring>
#include <string>

namespace backend::db::insert {
    template <typename ENTITY, typename... COMPONENTS>
    struct Query {
        static std::string Render();
    };

    template <typename ENTITY, typename... COMPONENTS>
    std::string Query::Render() {
        std::ostringstream ss;

    }
}

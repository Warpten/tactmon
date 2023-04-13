#pragma once

#include <string>
#include <sstream>
#include <utility>

#include "backend/db/orm/Concepts.hpp"

namespace backend::db {
    template <typename T>
    struct IQuery {
        static std::string render() {
            // static_assert(concepts::StreamRenderable<T>);

            std::stringstream ss;
            T::render_to(ss, std::integral_constant<std::size_t, 1> { });
            return ss.str();
        }
    };
}

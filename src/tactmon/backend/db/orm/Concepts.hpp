#pragma once

#include <concepts>
#include <cstdint>
#include <ostream>
#include <string>
#include <utility>

namespace backend::db::orm {
    namespace concepts {
        template <typename T>
        concept StreamRenderable = requires () {
            T::render_to(std::declval<std::ostream&>(), std::integral_constant<size_t, 1> { });
        };

        template <typename T>
        concept Renderable = requires () {
            { T::render(std::integral_constant<size_t, 1> { }) } -> std::same_as<std::string>;
        };
    }
}

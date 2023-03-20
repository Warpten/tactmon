#pragma once

#include "utility/Tuple.hpp"

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

        namespace detail {
            template <typename T> struct IsTuple : std::false_type { };
            template <typename... Ts> struct IsTuple<utility::tuple<Ts...>> : std::true_type { };
        }

        template <typename T>
        concept IsParameterized = detail::IsTuple<typename T::parameter_types>;
    }
}

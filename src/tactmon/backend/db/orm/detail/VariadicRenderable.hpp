#pragma once

#include "backend/db/orm/Concepts.hpp"
#include "utility/Literal.hpp"
#include "utility/Tuple.hpp"

#include <cstdint>
#include <ostream>
#include <type_traits>

namespace backend::db::detail {
    /**
     * An utility type that provides variadic rendering.
     *
     * @tparam TOKEN         A separator to be printed between individual components.
     * @tparam COMPONENTS... A sequence of components to be printed.
     */
    template <utility::Literal TOKEN, typename... COMPONENTS>
    struct VariadicRenderable final {
        template <std::size_t PARAMETER>
        constexpr static auto render_to(std::string prev, std::integral_constant<std::size_t, PARAMETER> p) {
            return render_v2_<0>(prev, p);
        }

    private:
        template <std::size_t I, std::size_t PARAMETER>
        constexpr static auto render_v2_(std::string prev, std::integral_constant<std::size_t, PARAMETER> p) {
            if constexpr (I >= sizeof...(COMPONENTS))
                return std::make_pair(prev, p);
            else if constexpr (I > 0) {
                return std::apply([](std::string const str, auto u) {
                    return render_v2_<I + 1>(str, u);
                }, utility::tuple_element_t<I, utility::tuple<COMPONENTS...>>::render_to(prev + TOKEN.Value, p));
            }
            else {
                return std::apply([](std::string const str, auto u) {
                    return render_v2_<I + 1>(str, u);
                }, utility::tuple_element_t<I, utility::tuple<COMPONENTS...>>::render_to(prev, p));
            }
        }
    };
}

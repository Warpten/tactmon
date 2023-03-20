#pragma once

#include "backend/db/orm/concepts/Concepts.hpp"
#include "utility/Literal.hpp"
#include "utility/Tuple.hpp"

#include <cstdint>
#include <ostream>
#include <type_traits>

namespace backend::db::orm {
    /**
     * An utility type that provides variadic rendering.
     *
     * @tparam TOKEN         A separator to be printed between individual components.
     * @tparam COMPONENTS... A sequence of components to be printed.
     */
    template <utility::Literal TOKEN, concepts::StreamRenderable... COMPONENTS>
    struct VariadicRenderable final {
        template <size_t PARAMETER>
        static auto render_to(std::ostream& stream, std::integral_constant<size_t, PARAMETER> p) {
            return render_<0>(stream, p);
        }

    private:
        template <size_t I, size_t PARAMETER>
        static auto render_(std::ostream& strm, std::integral_constant<size_t, PARAMETER> p) {
            if constexpr (I >= sizeof...(COMPONENTS))
                return p;
            else {
                if constexpr (I > 0)
                    strm << TOKEN.Value;

                auto ofs = utility::tuple_element_t<I, utility::tuple<COMPONENTS...>>::render_to(strm, p);
                return render_<I + 1>(strm, ofs);
            }
        }
    };
}

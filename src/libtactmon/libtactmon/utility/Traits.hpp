#pragma once

#include <cstdint>
#include <span>
#include <type_traits>

namespace libtactmon::utility {
    template <typename T> struct is_span : std::false_type { };
    template <typename T, std::size_t N> struct is_span<std::span<T, N>> : std::true_type { };

    template <typename T>
    constexpr static const bool is_span_v = is_span<T>::value;
}

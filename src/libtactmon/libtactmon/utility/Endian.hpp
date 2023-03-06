#pragma once

#include <array>
#include <bit>
#include <concepts>

namespace libtactmon::utility {
    template <typename To, typename From, typename = std::enable_if_t<sizeof(To) == sizeof(From)>>
    constexpr auto bit_cast(const From& from) noexcept -> To {
#ifdef __cpp_lib_bit_cast
        return std::bit_cast<To>(from);
#else
        auto to = To();
        std::memcpy(static_cast<void*>(std::addressof(to)), &from, sizeof(to));

        return to;
#endif
    }

#if __cpp_lib_byteswap >= 202110L
    using std::byteswap;
#else
    template <std::integral T>
    constexpr T byteswap(T value) noexcept {
        static_assert(std::has_unique_object_representations_v<T>, "T may not have padding bits");

        auto value_representation = bit_cast<std::array<uint8_t, sizeof(T)>>(value);

        std::ranges::reverse(value_representation);
        return utility::bit_cast<T>(value_representation);
    }
#endif

    template <std::integral T>
    constexpr T to_endianness(T value, std::endian from, std::endian to) {
        return (from == to) ? value : byteswap(value);
    }

    template <std::endian To, std::endian From = std::endian::native, typename T>
    constexpr T to_endianness(T value) {
        if constexpr (From == To)
            return value;

        return byteswap(value);
    }
}

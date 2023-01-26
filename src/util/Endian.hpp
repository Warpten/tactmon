#pragma once

#include <bit>
#include <concepts>

namespace util {
#if __cpp_lib_byteswap >= 202110L
	using std::byteswap;
#else
	template <std::integral T>
	constexpr T byteswap(T value) noexcept {
		static_assert(std::has_unique_object_representations_v<T>, "T may not have padding bits");
		auto value_representation = std::bit_cast<std::array<std::byte, sizeof(T)>>(value);
		std::ranges::reverse(value_representation);
		return std::bit_cast<T>(value_representation);
	}
#endif
}

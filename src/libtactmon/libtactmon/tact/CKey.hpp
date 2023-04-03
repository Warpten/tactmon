#pragma once

#include "libtactmon/detail/Export.hpp"
#include "libtactmon/tact/detail/Key.hpp"

#include <array>
#include <concepts>
#include <cstdint>
#include <memory>
#include <ranges>
#include <span>
#include <string>
#include <string_view>
#include <utility>

namespace libtactmon::tact {
    /**
     * Represents a content key.
     */
    struct LIBTACTMON_API CKey final : private detail::Key {
        /**
         * Tries to construct an content key from a hex string.
         * 
         * @param[in]  value  The hex string representation of this content key.
         * @param[out] target An instance of EKey.
         * 
         * @returns A boolean indicating wether or not parsing the input value was successful.
         */
        static bool TryParse(std::string_view value, CKey& target);

        using detail::Key::Key;
        using detail::Key::ToString;
        using detail::Key::data;

        friend bool operator == (CKey const& left, CKey const& right) noexcept;

        template <std::ranges::range T>
        friend bool operator == (CKey const& left, T right) noexcept {
            return std::equal(left._data.get(), left._data.get() + left._size, std::begin(right), std::end(right));
        }
    };
}

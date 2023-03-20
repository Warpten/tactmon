#pragma once

#include <array>
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
    struct CKey final {
        /**
         * Tries to construct an content key from a hex string.
         * 
         * @param[in]  value  The hex string representation of this content key.
         * @param[out] target An instance of EKey.
         * 
         * @returns A boolean indicating wether or not parsing the input value was successful.
         */
        static bool TryParse(std::string_view value, CKey& target);

        CKey();
        CKey(CKey&& other) noexcept;
        CKey(CKey const& other);

        CKey& operator = (CKey&& other) noexcept;
        CKey& operator = (CKey const& other);

        /**
        * Constructs a content key, copying the contents of the memory range provided.
        * 
        * @param[in] data A range of bytes.
        */
        explicit CKey(std::span<uint8_t const> data);

        /**
         * Returns a hex string representation of this content key.
         */
        std::string ToString() const;

        std::span<uint8_t const> data() const { return std::span<uint8_t const> { _data.get(), _size }; }

        friend bool operator == (CKey const& left, CKey const& right) noexcept;

        template <std::ranges::range T>
        friend bool operator == (CKey const& left, T right) noexcept {
            return std::equal(left._data.get(), left._data.get() + left._size, std::begin(right), std::end(right));
        }
    private:
        std::unique_ptr<uint8_t[]> _data;
        size_t _size = 0;
    };
}

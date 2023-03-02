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
     * Represents an encoding key.
     * 
     * You may be confused as to why this type is the exact copy of CKey? Well, these are effectively the same (a hex string), but I wanted
     * different type semantics.
     */
    struct EKey final {
        /**
         * Tries to construct an encoding key from a hex string.
         * 
         * @param[in]  value  The hex string representation of this encoding key.
         * @param[out] target An instance of EKey.
         * 
         * @returns A boolean indicating wether or not parsing the input value was successful.
         */
        static bool TryParse(std::string_view value, EKey& target);

        EKey();

        /**
         * Constructs a content key, copying the contents of the memory range provided.
         * 
         * @param[in] data A range of bytes.
         */
        explicit EKey(std::span<uint8_t> data);

        /**
         * Constructs a content key, copying the contents of the memory range provided.
         * 
         * @param[in] data A range of bytes.
         */
        explicit EKey(std::span<uint8_t const> data);

        /**
         * Returns a hex string representation of this content key.
         */
        std::string ToString() const;

        friend bool operator == (EKey const& left, EKey const& right) noexcept;

        std::span<uint8_t const> data() const { return std::span<uint8_t const> { _data.get(), _size }; }
        
        template <std::ranges::range T>
        friend bool operator == (EKey const& left, T right) noexcept {
            return std::equal(left._data.get(), left._data.get() + left._size, std::begin(right), std::end(right));
        }

    private:
        std::unique_ptr<uint8_t[]> _data;
        size_t _size = 0;
    };
}

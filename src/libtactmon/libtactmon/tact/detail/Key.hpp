#pragma once

#include <array>
#include <cstdint>
#include <memory>
#include <ranges>
#include <span>
#include <string>
#include <string_view>
#include <utility>

namespace libtactmon::tact::detail {
    struct Key {
        Key();

        Key(Key const& other);
        Key(Key&& other) noexcept;

        Key& operator = (Key const& other);
        Key& operator = (Key&& other) noexcept;

        explicit Key(std::span<uint8_t const> data);

        /**
         * Returns a hex string representation of this key.
         */
        std::string ToString() const;

        /**
         * Returns an immutable range over this key's bytes.
         */
        std::span<uint8_t const> data() const { return std::span<uint8_t const> { _data.get(), _size }; }

    protected:
        std::unique_ptr<uint8_t[]> _data;
        std::size_t _size = 0;
    };
}

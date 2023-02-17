#pragma once

#include <array>
#include <cstdint>
#include <span>
#include <string>
#include <string_view>
#include <utility>

#include <boost/algorithm/hex.hpp>
#include <boost/algorithm/string.hpp>

namespace tact {
    /**
     * Represents an encoding key.
     * 
     * You may be confused as to why this type is the exact copy of CKey? Well, these are effectively the same (a hex string), but I wanted
     * different type semantics.
     */
    struct EKey final {
        EKey();

        explicit EKey(std::string_view value);

        EKey(std::span<uint8_t> data);
        EKey(std::span<uint8_t const> data);

        template <size_t N>
        explicit EKey(std::array<uint8_t, N> data) : _data(data), _size(N) { }

        EKey(uint8_t* data, size_t length);

        std::string ToString() const;

        friend bool operator == (EKey const& left, EKey const& right) noexcept;

        std::span<uint8_t const> data() const { return std::span<uint8_t const> { _data.get(), _size }; }
        
        template <typename T>
        requires requires (T instance) {
            { std::begin(instance) };
            { std::end(instance) };
        }
        friend bool operator == (EKey const& left, T right) noexcept {
            return std::equal(left._data.get(), left._data.get() + left._size, std::begin(right), std::end(right));
        }

    private:
        std::unique_ptr<uint8_t[]> _data;
        size_t _size = 0;
    };
}

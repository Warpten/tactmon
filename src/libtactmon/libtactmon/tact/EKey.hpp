#pragma once

#include <array>
#include <cstdint>
#include <memory>
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
        static bool TryParse(std::string_view value, EKey& target);

        EKey();

        explicit EKey(std::span<uint8_t> data);
        explicit EKey(std::span<uint8_t const> data);

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

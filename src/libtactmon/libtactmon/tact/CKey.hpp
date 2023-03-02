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
     * Represents a content key.
     */
    struct CKey final {
        static bool TryParse(std::string_view value, CKey& target);

        CKey();
        CKey(CKey const& other);

        CKey& operator = (CKey const& other);

        explicit CKey(std::span<uint8_t> data);
        explicit CKey(std::span<uint8_t const> data);

        std::string ToString() const;

        std::span<uint8_t const> data() const { return std::span<uint8_t const> { _data.get(), _size }; }

        friend bool operator == (CKey const& left, CKey const& right) noexcept;

        template <typename T>
        requires requires (T instance) {
            { std::begin(instance) };
            { std::end(instance) };
        }
        friend bool operator == (CKey const& left, T right) noexcept {
            return std::equal(left._data.get(), left._data.get() + left._size, std::begin(right), std::end(right));
        }
    private:
        std::unique_ptr<uint8_t[]> _data;
        size_t _size = 0;
    };
}

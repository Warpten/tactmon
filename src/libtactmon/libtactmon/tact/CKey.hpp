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
        CKey();

        CKey(CKey const& other);

        CKey& operator = (CKey const& other);

        explicit CKey(std::string_view value);

        CKey(std::span<uint8_t> data);
        CKey(std::span<uint8_t const> data);

        template <size_t N>
        explicit CKey(std::array<uint8_t, N> data) : _data(data), _size(N) { }

        CKey(uint8_t* data, size_t length);
        CKey(uint8_t const* data, size_t length);

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

#include "libtactmon/tact/detail/Key.hpp"
#include "libtactmon/utility/Hex.hpp"

#include <algorithm>
#include <utility>

#include <boost/algorithm/string.hpp>

namespace libtactmon::tact::detail {
    Key::Key() : _data(nullptr), _size(0) { }

    Key::Key(std::span<uint8_t const> data) {
        _data = std::make_unique<uint8_t[]>(data.size());
        _size = data.size();

        std::copy_n(data.data(), _size, _data.get());
    }

    Key::Key(Key const& other) : _data(std::make_unique<uint8_t[]>(other._size)), _size(other._size) {
        std::copy_n(other._data.get(), other._size, _data.get());
    }


    Key::Key(Key&& other) noexcept : _size(other._size), _data(std::move(other._data)) {
        other._size = 0;
    }

    std::string Key::ToString() const {
        return libtactmon::utility::hex(std::span<const uint8_t> { _data.get(), _size });
    }

    Key& Key::operator = (Key&& other) noexcept {
        _data = std::move(other._data);
        _size = other._size;

        other._size = 0;
        return *this;
    }

    Key& Key::operator = (Key const& other) {
        _data = std::make_unique<uint8_t[]>(other._size);
        _size = other._size;

        std::copy_n(other._data.get(), other._size, _data.get());
        return *this;
    }
}

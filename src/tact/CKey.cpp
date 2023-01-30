#include "tact/CKey.hpp"

#include <utility>

#include <boost/algorithm/hex.hpp>
#include <boost/algorithm/string.hpp>

namespace tact {
    CKey::CKey() : _data(nullptr), _size(0) { }

    CKey::CKey(CKey const& other) : _data(std::make_unique<uint8_t[]>(other._size)), _size(other._size) {
        std::copy_n(other._data.get(), other._size, _data.get());
    }

    CKey::CKey(std::string_view value) {
        _data = std::make_unique<uint8_t[]>(value.size() / 2);
        _size = value.size() / 2;

        boost::algorithm::unhex(value.begin(), value.end(), _data.get());
    }

    CKey::CKey(std::span<uint8_t> data) {
        _data = std::make_unique<uint8_t[]>(data.size());
        _size = data.size();

        std::copy_n(data.data(), _size, _data.get());
    }

    CKey::CKey(std::span<uint8_t const> data) {
        _data = std::make_unique<uint8_t[]>(data.size());
        _size = data.size();

        std::copy_n(data.data(), _size, _data.get());
    }

    CKey::CKey(uint8_t* data, size_t length) {
        _data = std::make_unique<uint8_t[]>(length);
        _size = length;

        std::copy_n(data, length, _data.get());
    }

    CKey& CKey::operator = (CKey const& other) {
        _data = std::make_unique<uint8_t[]>(other._size);
        _size = other._size;

        std::copy_n(other._data.get(), other._size, _data.get());
        return *this;
    }

    std::string CKey::ToString() const {
        std::string value;
        value.reserve(_size * 2u);

        boost::algorithm::hex(_data.get(), _data.get() + _size, std::back_inserter(value));
        boost::algorithm::to_lower(value);

        return value;
    }

    bool operator == (CKey const& left, CKey const& right) noexcept {
        if (left._size != right._size)
            return false;

        return std::equal(left._data.get(), left._data.get() + left._size, right._data.get(), right._data.get() + right._size);
    }
}

#include "libtactmon/tact/CKey.hpp"

#include <utility>

#include <boost/algorithm/hex.hpp>
#include <boost/algorithm/string.hpp>

namespace libtactmon::tact {
    bool CKey::TryParse(std::string_view value, CKey& target) {
        // TODO: assert that the input target is empty

        target._size = value.size() / 2;
        target._data = std::make_unique<uint8_t[]>(target._size);
        
        auto itr = boost::algorithm::unhex(value.begin(), value.end(), target._data.get());
        if (itr != target._data.get() + target._size) {
            target._size = 0;
            target._data.release();

            return false;
        }

        return true;
    }

    CKey::CKey() : _data(nullptr), _size(0) { }

    CKey::CKey(CKey const& other) : _data(std::make_unique<uint8_t[]>(other._size)), _size(other._size) {
        std::copy_n(other._data.get(), other._size, _data.get());
    }

    CKey::CKey(CKey&& other) noexcept : _size(other._size), _data(std::move(other._data)) {
        other._size = 0;
    }

    CKey::CKey(std::span<uint8_t const> data) {
        _data = std::make_unique<uint8_t[]>(data.size());
        _size = data.size();

        std::copy_n(data.data(), _size, _data.get());
    }

    CKey& CKey::operator = (CKey&& other) noexcept {
        _data = std::move(other._data);
        _size = other._size;

        other._size = 0;
        return *this;
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

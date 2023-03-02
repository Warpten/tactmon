#include "libtactmon/tact/EKey.hpp"

#include <utility>

#include <boost/algorithm/hex.hpp>
#include <boost/algorithm/string.hpp>

namespace libtactmon::tact {
    bool EKey::TryParse(std::string_view value, EKey& target) {
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

    EKey::EKey() : _data(nullptr), _size(0) { }

    EKey::EKey(std::span<uint8_t> data) {
        _data = std::make_unique<uint8_t[]>(data.size());
        _size = data.size();

        std::copy_n(data.data(), _size, _data.get());
    }

    EKey::EKey(std::span<uint8_t const> data) {
        _data = std::make_unique<uint8_t[]>(data.size());
        _size = data.size();

        std::copy_n(data.data(), _size, _data.get());
    }

    std::string EKey::ToString() const {
        std::string value;
        value.reserve(_size * 2u);

        boost::algorithm::hex(_data.get(), _data.get() + _size, std::back_inserter(value));
        boost::algorithm::to_lower(value);

        return value;
    }

    bool operator == (EKey const& left, EKey const& right) noexcept {
        if (left._size != right._size)
            return false;

        return std::equal(left._data.get(), left._data.get() + left._size, right._data.get(), right._data.get() + right._size);
    }
}

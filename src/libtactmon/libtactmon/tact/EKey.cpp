#include "libtactmon/tact/EKey.hpp"
#include "libtactmon/utility/Hex.hpp"

#include <utility>

namespace libtactmon::tact {
    bool EKey::TryParse(std::string_view value, EKey& target) {
        target._size = value.size() / 2;
        target._data = std::make_unique<uint8_t[]>(target._size);

        libtactmon::utility::unhex(value, std::span<uint8_t> { target._data.get(), target._size });
        return true;
    }

    bool operator == (EKey const& left, EKey const& right) noexcept {
        if (left._size != right._size)
            return false;

        return std::equal(left._data.get(), left._data.get() + left._size, right._data.get(), right._data.get() + right._size);
    }
}

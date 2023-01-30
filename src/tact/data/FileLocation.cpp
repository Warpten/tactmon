#include "tact/data/FileLocation.hpp"

namespace tact::data {
    FileLocation::FileLocation(size_t fileSize, size_t keyCount, std::span<uint8_t> keys)
        : _fileSize(fileSize), _keyCount(keyCount), _keys(keys)
    { }

    std::span<const uint8_t> FileLocation::operator [] (size_t index) const {
        size_t keySize = _keys.size() / _keyCount;
        return _keys.subspan(index * keySize, keySize);
    }
}

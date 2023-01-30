#pragma once

#include <cstdint>
#include <span>

namespace tact::data {
    struct FileLocation final {
        explicit FileLocation(size_t fileSize, size_t keyCount, std::span<uint8_t> keys);

        size_t fileSize() const { return _fileSize; }
        size_t keyCount() const { return _keyCount; }
        std::span<const uint8_t> operator [] (size_t index) const;

    private:
        size_t _fileSize;
        size_t _keyCount;
        std::span<uint8_t> _keys;
    };
}

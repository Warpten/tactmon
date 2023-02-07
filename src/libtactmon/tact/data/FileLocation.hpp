#pragma once

#include <cstdint>
#include <span>
#include <string>
#include <string_view>

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

    struct IndexFileLocation final {
        explicit IndexFileLocation(std::string_view archiveName);
        explicit IndexFileLocation(std::string_view archiveName, size_t offset, size_t size);

        std::string_view name() const { return _archiveName; }
        size_t fileSize() const { return _fileSize; }
        size_t offset() const { return _offset; }

    private:
        size_t _fileSize = 0;
        size_t _offset = 0;
        std::string _archiveName;
    };
}

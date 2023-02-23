#pragma once

#include <cstdint>
#include <span>
#include <string>
#include <string_view>

namespace libtactmon::tact::data {
    /**
     * Describes the location of a given file, according to one or many encoding keys.
     */
    struct FileLocation final {
        explicit FileLocation(size_t fileSize, size_t keyCount, std::span<uint8_t> keys);

        /**
         * Returns the decompressed size of this file.
         */
        size_t fileSize() const { return _fileSize; }

        /**
         * Returns the amount of encoding keys for the associated file.
         */
        size_t keyCount() const { return _keyCount; }

        /**
         * Returns one of the encoding keys for the associated file.
         */
        std::span<const uint8_t> operator [] (size_t index) const;

    private:
        size_t _fileSize;
        size_t _keyCount;
        std::span<uint8_t> _keys;
    };

    /**
     * Represents the location of a specific file in an archive.
     */
    struct ArchiveFileLocation final {
        explicit ArchiveFileLocation(std::string_view archiveName);
        explicit ArchiveFileLocation(std::string_view archiveName, size_t offset, size_t size);

        /**
         * Returns the name of the archive.
         */
        std::string_view name() const { return _archiveName; }

        /**
         * Returns the compressed size of the file.
         */
        size_t fileSize() const { return _fileSize; }

        /**
         * Returns the offset of the file in the archive.
         */
        size_t offset() const { return _offset; }

    private:
        size_t _fileSize = 0;
        size_t _offset = 0;
        std::string _archiveName;
    };
}

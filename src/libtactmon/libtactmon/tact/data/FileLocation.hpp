#pragma once

#include "libtactmon/detail/Export.hpp"
#include "libtactmon/tact/EKey.hpp"

#include <cstdint>
#include <span>
#include <string>
#include <string_view>

namespace libtactmon::tact::data {
    /**
     * Describes the location of a given file, according to one or many encoding keys.
     */
    struct LIBTACTMON_API FileLocation final {
        explicit FileLocation(std::size_t fileSize, std::size_t keyCount, std::span<const uint8_t> keys);

        /**
         * Returns the decompressed size of this file.
         */
        [[nodiscard]] std::size_t fileSize() const { return _fileSize; }

        /**
         * Returns the amount of encoding keys for the associated file.
         */
        [[nodiscard]] std::size_t keyCount() const { return _keyCount; }

        /**
         * Returns one of the encoding keys for the associated file.
         */
        EKey operator [] (size_t index) const;

    private:
        std::size_t _fileSize;
        std::size_t _keyCount;
        std::span<const uint8_t> _keys;
    };

    /**
     * Represents the location of a specific file in an archive.
     */
    struct LIBTACTMON_API ArchiveFileLocation final {
        explicit ArchiveFileLocation(std::string_view archiveName);
        explicit ArchiveFileLocation(std::string_view archiveName, std::size_t offset, std::size_t size);

        /**
         * Returns the name of the archive.
         */
        [[nodiscard]] std::string_view name() const { return _archiveName; }

        /**
         * Returns the compressed size of the file.
         */
        [[nodiscard]] std::size_t fileSize() const { return _fileSize; }

        /**
         * Returns the offset of the file in the archive.
         */
        [[nodiscard]] std::size_t offset() const { return _offset; }

    private:
        std::size_t _fileSize = 0;
        std::size_t _offset = 0;
        std::string _archiveName;
    };
}

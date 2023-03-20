#include "libtactmon/tact/data/FileLocation.hpp"

#include <assert.hpp>

namespace libtactmon::tact::data {
    FileLocation::FileLocation(size_t fileSize, size_t keyCount, std::span<const uint8_t> keys)
        : _fileSize(fileSize), _keyCount(keyCount), _keys(keys)
    { }

    EKey FileLocation::operator [] (size_t index) const {
        DEBUG_ASSERT(index < _keyCount, "Out of range index value");

        size_t keySize = _keys.size() / _keyCount;
        return EKey { _keys.subspan(index * keySize, keySize) };
    }

    ArchiveFileLocation::ArchiveFileLocation(std::string_view archiveName) : _archiveName(archiveName) { }

    ArchiveFileLocation::ArchiveFileLocation(std::string_view archiveName, size_t offset, size_t size)
        : _archiveName(archiveName), _fileSize(size), _offset(offset)
    { }
}

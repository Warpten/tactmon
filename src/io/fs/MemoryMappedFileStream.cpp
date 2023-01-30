#include "io/fs/MemoryMappedFileStream.hpp"

namespace io {
    MemoryMappedFileStream::MemoryMappedFileStream(std::filesystem::path filePath, std::endian fileEndianness)
        : IStream(fileEndianness), _stream(filePath.string())
    { }

    size_t MemoryMappedFileStream::SeekRead(size_t offset) {
        return _cursor = offset;
    }

    size_t MemoryMappedFileStream::GetReadCursor() const {
        return _cursor;
    }

    size_t MemoryMappedFileStream::GetLength() const {
        return _stream.size();
    }

    size_t MemoryMappedFileStream::_ReadImpl(std::span<std::byte> bytes) {
        size_t length = std::min(bytes.size(), _stream.size() - _cursor);

        std::memcpy(bytes.data(), _stream.data() + _cursor, length);
        _cursor += length;
        return length;
    }
}
#include "io/fs/FileStream.hpp"

namespace io {
    FileStream::FileStream(std::filesystem::path filePath, std::endian fileEndianness)
        : IStream(fileEndianness), _stream(filePath.string())
    { }

    size_t FileStream::SeekRead(size_t offset) {
        return _cursor = offset;
    }

    size_t FileStream::GetReadCursor() const {
        return _cursor;
    }

    size_t FileStream::GetLength() const {
        return _stream.size();
    }

    size_t FileStream::_ReadImpl(std::span<std::byte> bytes) {
        size_t length = std::min(bytes.size(), _stream.size() - _cursor);

        std::memcpy(bytes.data(), _stream.data() + _cursor, length);
        _cursor += length;
        return length;
    }
}

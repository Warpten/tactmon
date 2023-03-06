#include "libtactmon/io/FileStream.hpp"

namespace libtactmon::io {
    FileStream::FileStream(std::filesystem::path filePath)
        : IStream()
    {
        try {
            _stream.open(filePath.string());
        } catch (...) {
            // Failed to open the file, probably does not exist, is empty, or invalid
        }
    }

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

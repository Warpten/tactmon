#include "libtactmon/io/FileStream.hpp"

namespace libtactmon::io {
    FileStream::FileStream(const std::filesystem::path& filePath)
        : IStream()
    {
        try {
            _stream.open(filePath.string());
        } catch (...) {
            // Failed to open the file, probably does not exist, is empty, or invalid
        }
    }

    std::size_t FileStream::SeekRead(std::size_t offset) {
        return _cursor = offset;
    }

    std::size_t FileStream::GetReadCursor() const {
        return _cursor;
    }

    std::size_t FileStream::GetLength() const {
        return _stream.size();
    }

    std::size_t FileStream::_ReadImpl(std::span<std::byte> bytes) {
        std::size_t length = std::min(bytes.size(), _stream.size() - _cursor);

        std::memcpy(bytes.data(), _stream.data() + _cursor, length);
        _cursor += length;
        return length;
    }
}

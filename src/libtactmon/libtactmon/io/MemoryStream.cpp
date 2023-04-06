#include "libtactmon/io/MemoryStream.hpp"

namespace libtactmon::io {
    SpanStream::SpanStream(std::span<const std::byte> data) : _data(data) { }

    std::size_t SpanStream::_ReadImpl(std::span<std::byte> bytes) {
        std::size_t length = std::min(bytes.size(), _data.size() - _cursor);

        std::copy_n(Data().subspan(length).data(), length, bytes.data());
        _cursor += length;
        return length;
    }

    MemoryStream::MemoryStream(std::span<std::byte> data)
        : IReadableStream(), _data(data)
    { }

    std::size_t MemoryStream::SeekRead(std::size_t offset) {
        _cursor = std::min(_data.size(), offset);

        return _cursor;
    }

    std::size_t MemoryStream::_ReadImpl(std::span<std::byte> bytes) {
        std::size_t length = std::min(bytes.size(), _data.size() - _cursor);

        bytes = std::span { reinterpret_cast<std::byte*>(_data.data()) + _cursor, length };
        _cursor += length;
        return length;
    }

    // ^^^ MemoryStream / GrowableMemoryStream vvv

    GrowableMemoryStream::GrowableMemoryStream() : IReadableStream(), IWritableStream() { }

    GrowableMemoryStream::GrowableMemoryStream(std::span<std::byte> data)
        : IReadableStream(), IWritableStream(), _data(data.begin(), data.end())
    { }

    std::size_t GrowableMemoryStream::SeekRead(std::size_t offset) {
        return _readCursor = std::min(offset, _data.size());
    }

    std::size_t GrowableMemoryStream::SeekWrite(std::size_t offset) {
        _writeCursor = offset;
        _data.resize(offset);
        return _writeCursor;
    }

    std::size_t GrowableMemoryStream::_ReadImpl(std::span<std::byte> bytes) {
        std::size_t length = std::min(bytes.size(), _data.size() - _readCursor);

        std::copy_n(Data().data(), length, bytes.begin());
        _readCursor += length;
        return length;
    }

    std::span<std::byte> GrowableMemoryStream::_WriteImpl(std::span<const std::byte> bytes) {
        if (_data.size() < _writeCursor + bytes.size())
            _data.resize(_writeCursor + bytes.size());

        std::span<std::byte> writtenData { _data.data() + _writeCursor, bytes.size() };

        std::memcpy(writtenData.data(), bytes.data(), bytes.size());
        _writeCursor += bytes.size();
        return writtenData;
    }
}

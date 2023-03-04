#pragma once

#include "libtactmon/io/IStream.hpp"
#include "libtactmon/io/IReadableStream.hpp"
#include "libtactmon/io/IWritableStream.hpp"

#include <bit>
#include <cstdint>
#include <span>
#include <string>
#include <vector>

#include <boost/asio/buffer.hpp>
#include <boost/beast/core/buffer_traits.hpp>
#include <boost/beast/core/buffers_range.hpp>
#include <boost/beast/core/buffers_to_string.hpp>

namespace libtactmon::io {
    /**
     * An implementation of the Stream interface.
     */
    struct MemoryStream final : IReadableStream {
        MemoryStream(std::span<std::byte> data, std::endian endianness);

        size_t GetReadCursor() const override { return _cursor; }
        size_t SeekRead(size_t offset) override;
        void SkipRead(size_t offset) override { _cursor += offset; }
        bool CanRead(size_t amount) const override { return _cursor + amount <= _data.size(); }
        size_t GetLength() const override { return _data.size(); }

        operator bool() const override { return true; }

        std::span<std::byte const> Data() const override { return std::span { _data }.subspan(_cursor); }

    protected:
        size_t _ReadImpl(std::span<std::byte> writableSpan) override;

    private:
        std::span<std::byte> _data;
        size_t _cursor = 0;
    };

    struct GrowableMemoryStream final : IReadableStream, IWritableStream {
        explicit GrowableMemoryStream(std::endian endianness);
        GrowableMemoryStream(std::span<std::byte> data, std::endian endianness);

        template <typename ConstBufferSequence>
        requires boost::asio::is_const_buffer_sequence<ConstBufferSequence>::value
        GrowableMemoryStream(ConstBufferSequence const& buffers, std::endian endianness) : GrowableMemoryStream(endianness) {
            _data.reserve(boost::asio::buffer_size(buffers));
            for (auto const buffer : boost::beast::buffers_range_ref(buffers))
                _data.insert(_data.end(), reinterpret_cast<const std::byte*>(buffer.data()), reinterpret_cast<const std::byte*>(buffer.data()) + buffer.size());
        }

        void Consume(size_t bytes);

        size_t GetLength() const override { return _data.size(); }
        operator bool() const override { return true; }

    public:
        size_t GetReadCursor() const override { return _readCursor; }
        size_t SeekRead(size_t offset) override;
        void SkipRead(size_t offset) override { _readCursor += offset; }
        bool CanRead(size_t amount) const override { return _readCursor + amount <= _data.size(); }
        std::span<std::byte const> Data() const override { return std::span { _data }.subspan(_readCursor); }

        using IReadableStream::Data;

    public:
        size_t GetWriteCursor() const override { return _writeCursor; }
        size_t SeekWrite(size_t offset) override;
        void SkipWrite(size_t offset) override { _writeCursor += offset; }

    protected:
        size_t _ReadImpl(std::span<std::byte> writableSpan) override;
        std::span<std::byte> _WriteImpl(std::span<const std::byte> writableSpan) override;

    private:
        std::vector<std::byte> _data;
        size_t _readCursor = 0;
        size_t _writeCursor = 0;
    };
}

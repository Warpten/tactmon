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
    struct SpanStream final : IReadableStream {
        explicit SpanStream(std::span<const std::byte> data);

        using IReadableStream::Data;

    public: // IReadableStream
        [[nodiscard]] std::size_t GetReadCursor() const override { return _cursor; }
        std::size_t SeekRead(std::size_t offset) override { return _cursor = std::min(offset, _data.size()); }
        void SkipRead(std::size_t offset) override { _cursor += offset; }
        [[nodiscard]] bool CanRead(std::size_t amount) const override { return _cursor + amount <= _data.size(); }
        [[nodiscard]] std::size_t GetLength() const override { return _data.size(); }

        explicit operator bool() const override { return true; }

        [[nodiscard]] std::span<std::byte const> Data() const override { return _data.subspan(_cursor); }

    protected:
        std::size_t _ReadImpl(std::span<std::byte> writableSpan) override;

    private:
        std::span<const std::byte> _data;
        std::size_t _cursor = 0;
    };

    /**
     * An implementation of the Stream interface.
     */
    struct MemoryStream final : IReadableStream {
        explicit MemoryStream(std::span<std::byte> data);

        [[nodiscard]] std::size_t GetReadCursor() const override { return _cursor; }
        std::size_t SeekRead(std::size_t offset) override;
        void SkipRead(std::size_t offset) override { _cursor += offset; }
        [[nodiscard]] bool CanRead(std::size_t amount) const override { return _cursor + amount <= _data.size(); }
        [[nodiscard]] std::size_t GetLength() const override { return _data.size(); }

        explicit operator bool() const override { return true; }

        [[nodiscard]] std::span<std::byte const> Data() const override { return std::span { _data }.subspan(_cursor); }

    protected:
        std::size_t _ReadImpl(std::span<std::byte> writableSpan) override;

    private:
        std::span<std::byte> _data;
        std::size_t _cursor = 0;
    };

    struct GrowableMemoryStream final : IReadableStream, IWritableStream {
        explicit GrowableMemoryStream();
        explicit GrowableMemoryStream(std::span<std::byte> data);

        template <typename ConstBufferSequence>
        requires boost::asio::is_const_buffer_sequence<ConstBufferSequence>::value
        explicit GrowableMemoryStream(ConstBufferSequence const& buffers) : GrowableMemoryStream() {
            _data.reserve(boost::asio::buffer_size(buffers));
            for (auto const buffer : boost::beast::buffers_range_ref(buffers))
                _data.insert(_data.end(), reinterpret_cast<const std::byte*>(buffer.data()), reinterpret_cast<const std::byte*>(buffer.data()) + buffer.size());
        }

        [[nodiscard]] std::size_t GetLength() const override { return _data.size(); }
        explicit operator bool() const override { return true; }

    public:
        [[nodiscard]] std::size_t GetReadCursor() const override { return _readCursor; }
        std::size_t SeekRead(std::size_t offset) override;
        void SkipRead(std::size_t offset) override { _readCursor += offset; }
        [[nodiscard]] bool CanRead(std::size_t amount) const override { return _readCursor + amount <= _data.size(); }
        [[nodiscard]] std::span<std::byte const> Data() const override { return std::span { _data }.subspan(_readCursor); }

        using IReadableStream::Data;

    public:
        std::size_t GetWriteCursor() const override { return _writeCursor; }
        std::size_t SeekWrite(std::size_t offset) override;
        void SkipWrite(std::size_t offset) override { _writeCursor += offset; }

    protected:
        std::size_t _ReadImpl(std::span<std::byte> writableSpan) override;
        std::span<std::byte> _WriteImpl(std::span<const std::byte> writableSpan) override;

    private:
        std::vector<std::byte> _data;
        std::size_t _readCursor = 0;
        std::size_t _writeCursor = 0;
    };
}

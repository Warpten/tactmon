#pragma once

#include "io/IStream.hpp"

#include <bit>
#include <cstdint>
#include <span>
#include <string>
#include <vector>

namespace io::mem {
    /**
     * An implementation of the Stream interface.
     */
    struct MemoryStream final : IReadableStream {
        MemoryStream(std::span<uint8_t> data, std::endian endianness);

        std::span<const uint8_t> AsSpan() const;

        size_t GetReadCursor() const override { return _cursor; }
        size_t SeekRead(size_t offset) override;
        void SkipRead(size_t offset) override { _cursor += offset; }
        bool CanRead(size_t amount) const override { return _cursor + amount <= _data.size(); }

    protected:
        size_t _ReadImpl(std::span<std::byte> writableSpan) override;

    private:
        std::span<uint8_t> _data;
        size_t _cursor = 0;
    };

    struct GrowableMemoryStream final : IReadableStream, IWritableStream {
        explicit GrowableMemoryStream(std::endian endianness);
        GrowableMemoryStream(std::span<uint8_t> data, std::endian endianness);

        operator bool() const override { return true; }

        std::span<const uint8_t> AsSpan() const;

        size_t GetLength() const override { return _data.size(); }

    public:
        size_t GetReadCursor() const override { return _readCursor; }
        size_t SeekRead(size_t offset) override;
        void SkipRead(size_t offset) override { _readCursor += offset; }
        bool CanRead(size_t amount) const override { return _readCursor + amount <= _data.size(); }

    public:
        size_t GetWriteCursor() const override { return _writeCursor; }
        size_t SeekWrite(size_t offset) override;
        void SkipWrite(size_t offset) override { _writeCursor += offset; }

    protected:
        size_t _ReadImpl(std::span<std::byte> writableSpan) override;
        size_t _WriteImpl(std::span<std::byte> writableSpan) override;

    private:
        std::vector<uint8_t> _data;
        size_t _readCursor = 0;
        size_t _writeCursor = 0;
    };
}
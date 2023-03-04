#pragma once

#include "libtactmon/io/IStream.hpp"
#include "libtactmon/utility/Traits.hpp"

namespace libtactmon::io {
    /**
     * Implementation of a writable stream.
     */
    struct IWritableStream : virtual IStream {
        explicit IWritableStream(std::endian endianness = std::endian::native) : IStream(endianness) { }
        virtual ~IWritableStream() = default;

        virtual size_t GetWriteCursor() const = 0;
        virtual size_t SeekWrite(size_t offset) = 0;

        virtual void SkipWrite(size_t offset) = 0;

        template <typename T>
        size_t Write(T const value, std::endian endianness) {
            if constexpr (utility::is_span_v<T>) {
                return _WriteSpan(std::as_bytes(value), endianness, sizeof(T));
            } else {
                std::span<const T> valueSpan { std::addressof(value), 1 };
                return _WriteSpan(valueSpan, endianness, sizeof(T));
            }
        }

        size_t WriteString(std::string_view value) {
            return Write(std::span { value }, GetEndianness());
        }

        size_t WriteCString(std::string_view value) {
            size_t bytesWritten = WriteString(value);
            if (bytesWritten == value.length())
                bytesWritten += Write(uint8_t(0), GetEndianness());
            return bytesWritten;
        }

    protected:
        virtual std::span<std::byte> _WriteImpl(std::span<const std::byte> value) = 0;

    private:
        template <typename T>
        size_t _WriteSpan(std::span<const T> valueSpan, std::endian endianness, size_t elementSize) {
            size_t writeCursor = GetWriteCursor();
            std::span<std::byte> writtenBytes = _WriteImpl(std::as_bytes(valueSpan));

            if (endianness != GetEndianness()) {
                auto begin = writtenBytes.begin();
                auto end = writtenBytes.end();

                while (begin != end) {
                    auto tail = begin;
                    std::advance(tail, elementSize);

                    std::ranges::reverse(begin, tail);
                    begin = tail;
                }
            }

            return writtenBytes.size();
        }
    };
}

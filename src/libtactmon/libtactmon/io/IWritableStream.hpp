#pragma once

#include "libtactmon/io/IStream.hpp"
#include "libtactmon/utility/Traits.hpp"

namespace libtactmon::io {
    /**
     * Provides write operations on a sequence of bytes.
     */
    struct IWritableStream : virtual IStream {
        explicit IWritableStream() { }
        virtual ~IWritableStream() = default;

        /**
         * Returns the position of the write cursor within the stream.
         */
        virtual std::size_t GetWriteCursor() const = 0;

        /**
         * Sets the position of the write cursor.
         * 
         * @param[in] offset The desired position of the write cursor.
         * @returns The new position of the write cursor.
         */
        virtual std::size_t SeekWrite(std::size_t offset) = 0;
        
        /**
         * Moves the write cursor by a specific offset.
         * 
         * @param[in] offset The amount of bytes by which to shift the write cursor.
         */
        virtual void SkipWrite(std::size_t offset) = 0;

        /**
         * Writes a value to the stream, with the given endianness.
         * 
         * @param[in] value      The value to write to the stream.
         * @param[in] endianness The endianness to use. By default, the native endianness is used.
         */
        template <typename T> requires (utility::is_span_v<T> || std::is_trivial_v<T>)
        std::size_t Write(T const value, std::endian endianness = std::endian::native) {
            if constexpr (utility::is_span_v<T>) {
                return _WriteSpan(std::as_bytes(value), endianness, sizeof(T));
            } else {
                std::span<const T> valueSpan { std::addressof(value), 1 };
                return _WriteSpan(valueSpan, endianness, sizeof(T));
            }
        }

        std::size_t WriteString(std::string_view value) {
            return Write(std::span { value });
        }

        std::size_t WriteCString(std::string_view value) {
            std::size_t bytesWritten = WriteString(value);
            if (bytesWritten == value.length())
                bytesWritten += Write(uint8_t(0));
            return bytesWritten;
        }

    protected:
        virtual std::span<std::byte> _WriteImpl(std::span<const std::byte> value) = 0;

    private:
        template <typename T>
        std::size_t _WriteSpan(std::span<const T> valueSpan, std::endian endianness, std::size_t elementSize) {
            std::span<std::byte> writtenBytes = _WriteImpl(std::as_bytes(valueSpan));

            if (endianness != std::endian::native) {
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

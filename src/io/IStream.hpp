#pragma once

#include <algorithm>
#include <bit>
#include <cstdint>
#include <memory>
#include <span>
#include <string>
#include <type_traits>
#include <vector>

namespace io {
    namespace detail {
        template <typename T> struct is_span : std::false_type { };
        template <typename T, size_t N> struct is_span<std::span<T, N>> : std::true_type { };
    }

    struct IStream {
        explicit IStream(std::endian endianness = std::endian::native) : _endianness(endianness) { }

        std::endian GetEndianness() const { return _endianness; }

        /**
         * Returns the length of this stream.
         */
        virtual size_t GetLength() const = 0;

        /**
         * Determines if this stream was successfully opened.
         */
        virtual operator bool() const = 0;

    private:
        std::endian _endianness;
    };

    /**
     * Implementation of a writable stream.
     */
    struct IWritableStream : virtual IStream {
        explicit IWritableStream(std::endian endianness = std::endian::native) : IStream(endianness) { }
        virtual ~IWritableStream() = default;

        virtual size_t GetWriteCursor() const = 0;
        virtual size_t SeekWrite(size_t offset) = 0;

        template <typename T>
        size_t Write(T value, std::endian endianness) {
            if constexpr (detail::is_span<T>::value) {
                return _WriteSpan(value, endianness, sizeof(typename T::value_type));
            } else {
                std::span<T> valueSpan{ std::addressof(value), 1 };

                return _WriteSpan(valueSpan, endianness, sizeof(T));
            }
        }

        size_t WriteString(std::string_view value) {
            return Write(std::span<char> { const_cast<char*>(value.data()), value.size() }, GetEndianness());
        }

        size_t WriteCString(std::string_view value) {
            size_t bytesWritten = WriteString(value);
            if (bytesWritten == value.length())
                bytesWritten += Write(uint8_t(0), GetEndianness());
            return bytesWritten;
        }

    protected:
        virtual size_t _WriteImpl(std::span<std::byte> value) = 0;

    private:
        template <typename T>
        size_t _WriteSpan(std::span<T> valueSpan, std::endian endianness, size_t elementSize) {
            std::span<std::byte> byteSpan = std::as_writable_bytes(valueSpan);

            if (endianness != GetEndianness()) {
                auto begin = std::ranges::begin(byteSpan);
                auto end = std::ranges::end(byteSpan);

                while (begin != end) {
                    auto tail = begin;
                    std::advance(tail, elementSize);

                    std::ranges::reverse(begin, tail);
                    begin = tail;
                }
            }

            return _WriteImpl(byteSpan);
        }
    };

    /**
     * Implementation of a readable stream.
     */
    struct IReadableStream : virtual IStream {
        explicit IReadableStream(std::endian endianness = std::endian::native) : IStream(endianness) { }
        virtual ~IReadableStream() = default;

        /**
         * Returns the position of the read cursor.
         */
        virtual size_t GetReadCursor() const = 0;

        /**
         * Sets the position of the read cursor.
         *
         * @param[in] offset The desired position of the read cursor.
         * @returns The new position of the read cursor.
         */
        virtual size_t SeekRead(size_t offset) = 0;

        /**
         * Reads a value from the stream, using the given endianness.
         * 
         * @param[in] value      The value to read.
         * @param[in] endianness The endianness to use.
         * @eturns The amount of bytes that were effectively read.
         */
        template <typename T>
        size_t Read(T& value, std::endian endianness) {
            if constexpr (detail::is_span<T>::value) {
                return _ReadSpan(value, endianness, sizeof(typename T::value_type));
            } else {
                return _ReadSpan(std::span<T> { std::addressof(value), 1 }, endianness, sizeof(T));
            }
        }

        template <typename T>
        size_t Read(std::vector<T>& value, std::endian endianness) {
            size_t readCount = 0;
            for (size_t i = 0; i < value.size(); ++i)
                readCount += Read(value[i], endianness);

            return readCount;
        }

        template <typename T>
        requires std::integral<T>
        T Read(std::endian endianness) {
            T value { };
            Read(value, endianness);
            return value;
        }

        size_t ReadString(std::string& value, size_t length) {
            std::unique_ptr<char[]> buffer = std::make_unique<char[]>(length);
            std::span<char> bufferView{ buffer.get(), length };

            size_t bytesRead = Read(bufferView, GetEndianness());
            value.assign(buffer.get(), bytesRead);
            return bytesRead;
        }

        size_t ReadCString(std::string& value) {
            char elem { };
            do {
                Read(elem, GetEndianness());
                if (elem == '\0')
                    break;

                value += elem;
            } while (true);

            value.shrink_to_fit();
            return value.length();
        }

    protected:
        virtual size_t _ReadImpl(std::span<std::byte> writableSpan) = 0;

    private:
        /**
         * Implements read operations.
         * 
         * @param[in] writableSpan A span that will receive the bytes read.
         * @param[in] endianness   The endianness to use when reading the value.
         * @param[in] elementSize  The primitive size to use when handling endianness.
         */
        template <typename T>
        size_t _ReadSpan(std::span<T> writableSpan, std::endian endianness, size_t elementSize) {
            if constexpr (std::is_same_v<std::byte, T>) {
                size_t readCount = _ReadImpl(writableSpan);

                if (endianness != GetEndianness()) {
                    auto begin = std::ranges::begin(writableSpan);
                    auto end = begin;
                    std::advance(end, readCount); // Not using std::ranges::end in case we didn't read all the bytes.

                    while (begin != end) {
                        auto tail = begin;
                        std::advance(tail, elementSize);

                        std::ranges::reverse(begin, tail);
                        begin = tail;
                    }
                }

                return readCount;
            } else {
                // Cast to std::span<std::byte> and read again
                std::span<std::byte> byteSpan = std::as_writable_bytes(writableSpan);
                return _ReadSpan(byteSpan, endianness, sizeof(T));
            }
        }
    };
}
#pragma once

#include "libtactmon/io/IStream.hpp"
#include "libtactmon/utility/Traits.hpp"

#include <bit>
#include <cstdint>
#include <iterator>
#include <memory>
#include <span>
#include <type_traits>
#include <vector>

namespace libtactmon::io {
    /**
     * Provides read operations on a sequence of bytes.
     */
    struct IReadableStream : virtual IStream {
        explicit IReadableStream() { }
        virtual ~IReadableStream() = default;

        /**
         * Moves the read cursor by a given offset.
         */
        virtual void SkipRead(size_t offset) = 0;

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
         * Returns true if at least @pre amount bytes can be read from the stream.
         */
        virtual bool CanRead(size_t amount) const = 0;

        /**
         * Returns a pointer to the data located at offset specified by the read cursor.
         *
         * Warning: the data returned by this function is using the endianness of the platform.
         */
        virtual std::span<std::byte const> Data() const = 0;

        template <typename T> requires (!std::same_as<T, std::byte> && std::is_trivial_v<T>)
        std::span<const T> Data() const {
            return std::span { reinterpret_cast<const T*>(Data().data()), Data().size() / sizeof(T) };
        }

        /**
         * Reads a value from the stream, using the given endianness.
         *
         * @param[in] value      The value to read.
         * @param[in] endianness The endianness to use. By default, the endianness of the current platform is used.
         * @eturns The amount of bytes that were effectively read.
         */
        template <typename T, typename = std::enable_if_t<!utility::is_span_v<T> && std::is_trivial_v<T>>>
        size_t Read(T& value, std::endian endianness = std::endian::native) {
            return _ReadSpan(std::span<T> { std::addressof(value), 1 }, endianness, sizeof(T));
        }

        /**
         * Reads a span of values from the stream, using the given endianness.
         * 
         * @param[in] span       A span around the values to read to.
         * @param[in] endianness The endianness to use. By default, the endianness of the current platform is used.
         * 
         * @returns The amount of bytes read.
         */
        template <typename T> requires (std::is_trivial_v<T>)
        size_t Read(std::span<T> span, std::endian endianness = std::endian::native) {
            return _ReadSpan(span, endianness, sizeof(T));
        }

        template <typename T>
        size_t Read(std::vector<T>& value, std::endian endianness = std::endian::native) {
            return _ReadSpan(std::span<T> { value }, endianness, sizeof(T));
        }

        template <typename T>
        requires std::integral<T>
        T Read(std::endian endianness = std::endian::native) {
            T value { };
            Read(value, endianness);
            return value;
        }

        size_t ReadString(std::string& value, size_t length) {
            // TODO: there probably is a way to avoid double copies here

            std::unique_ptr<char[]> buffer = std::make_unique<char[]>(length);
            std::span<char> bufferView { buffer.get(), length };

            size_t bytesRead = Read(bufferView);
            value.assign(buffer.get(), bytesRead);
            return bytesRead;
        }

        size_t ReadCString(std::string& value) {
            char elem{ };
            do {
                Read(elem);
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

                if (endianness != std::endian::native) {
                    auto begin = std::ranges::begin(writableSpan);
                    auto end = std::next(begin, readCount);

                    while (begin != end) {
                        auto tail = std::next(begin, elementSize);
                        std::ranges::reverse(begin, tail);
                        begin = tail;
                    }
                }

                return readCount;
            } else {
                // Cast to std::span<std::byte> and read again
                return _ReadSpan(std::as_writable_bytes(writableSpan), endianness, sizeof(T));
            }
        }
    };
}

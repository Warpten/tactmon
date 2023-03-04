#pragma once

#include <algorithm>
#include <bit>
#include <cstdint>
#include <memory>
#include <span>
#include <string>
#include <type_traits>
#include <vector>

namespace libtactmon::io {
    namespace detail {
        template <typename T> struct is_span : std::false_type { };
        template <typename T, size_t N> struct is_span<std::span<T, N>> : std::true_type { };

        template <typename T>
        constexpr static const bool is_span_v = is_span<T>::value;
    }

    /**
     * Encapsulates a sequence of bytes.
     */
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

}

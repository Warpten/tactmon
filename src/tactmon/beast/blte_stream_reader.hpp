#pragma once

#include <array>
#include <cstdint>
#include <functional>
#include <system_error>
#include <vector>

#include <boost/beast/core/error.hpp>

#include <libtactmon/io/MemoryStream.hpp>

namespace boost::beast::user {
    struct blte_stream_reader final {
        blte_stream_reader(blte_stream_reader const&) = delete;
        blte_stream_reader(blte_stream_reader&&) noexcept = delete;

        ~blte_stream_reader() = default;

        explicit blte_stream_reader();

        std::size_t write_some(uint8_t* data, size_t size, std::function<void(std::span<const uint8_t>)> acceptor, boost::beast::error_code& ec);

    private:
        enum step : uint32_t {
            header,
            chunk_info,
            // From here N data_blocks entries where N is the amount of actual chunks
            // Don't add steps after this
            data_blocks
        };

        struct chunk_info_t {
            uint32_t compressedSize;
            uint32_t decompressedSize;
            std::array<uint8_t, 16> checksum;
        };

        std::vector<chunk_info_t> _chunks;

        step _step = step::header;
        libtactmon::io::GrowableMemoryStream _ms;

        uint32_t _headerSize = 0;
    };
}

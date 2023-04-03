#pragma once

#include <array>
#include <cstdint>
#include <functional>
#include <memory>
#include <system_error>
#include <vector>

#include <boost/beast/core/error.hpp>

#include <libtactmon/crypto/Hash.hpp>
#include <libtactmon/io/MemoryStream.hpp>

namespace boost::beast::user {
    struct BlockTableEncodedStreamTransform final : std::enable_shared_from_this<BlockTableEncodedStreamTransform> {
        using OutputHandler = std::function<void(std::span<const uint8_t>)>;
        using InputFeedback = std::function<void(size_t)>;

        BlockTableEncodedStreamTransform(BlockTableEncodedStreamTransform const&) = delete;
        BlockTableEncodedStreamTransform(BlockTableEncodedStreamTransform&&) noexcept = delete;

        ~BlockTableEncodedStreamTransform() = default;

        BlockTableEncodedStreamTransform(OutputHandler handler, InputFeedback feedback);

        std::size_t Parse(uint8_t const* data, size_t size, boost::beast::error_code& ec);

    private:
        enum Step : uint32_t {
            Header,
            ChunkHeaders,
            // From here N data_blocks entries where N is the amount of actual chunks
            // Don't add steps after this
            DataBlocks
        };

        struct ChunkInfo {
            uint32_t compressedSize;
            uint32_t decompressedSize;
            std::array<uint8_t, 16> checksum;
        };

        std::vector<ChunkInfo> _chunks;

        Step _step = Step::Header;
        uint32_t _headerSize = 0;
        OutputHandler _handler;
        InputFeedback _feedback;
        libtactmon::io::GrowableMemoryStream _ms;
        crypto::MD5 _engine;
    };
}

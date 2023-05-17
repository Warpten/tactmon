#include "libtactmon/crypto/Hash.hpp"
#include "libtactmon/io/BlockTableEncodedStreamTransform.hpp"
#include "libtactmon/io/MemoryStream.hpp"
#include "libtactmon/utility/Endian.hpp"

#include <array>

#include <zlib.h>

namespace libtactmon::io {
    using namespace libtactmon::errors;

    // TODO: validate content using the content key as well
    Error BlockTableEncodedStreamTransform::operator () (IReadableStream& sourceStream, IWritableStream& targetStream, libtactmon::tact::EKey const* encodingKey, libtactmon::tact::CKey const* contentKey, bool validateKeys) const noexcept {
        if (!sourceStream.CanRead(4 + 4))
            return blte::MalformedArchive(encodingKey);

        uint32_t magic = sourceStream.Read<uint32_t>(std::endian::big);
        if (magic != 'BLTE')
            return blte::MalformedArchive(encodingKey);

        uint32_t headerSize = sourceStream.Read<uint32_t>(std::endian::big);
        if (!sourceStream.CanRead(headerSize))
            return blte::MalformedArchive(encodingKey);

        uint32_t flagsChunkCount = sourceStream.Read<uint32_t>(std::endian::big);

        // Validate the encoding key.
        crypto::MD5 engine;
        engine.UpdateData(utility::byteswap(magic));
        engine.UpdateData(utility::byteswap(headerSize));
        engine.UpdateData(utility::byteswap(flagsChunkCount));

        uint8_t flags = (flagsChunkCount & 0xFF000000) >> 24;
        uint32_t chunkCount = flagsChunkCount & 0x00FFFFFF;

        std::vector<std::function<errors::Error(IReadableStream&, IWritableStream&)>> chunkHandlers;
        for (std::size_t i = 0; i < chunkCount; ++i) {
            uint32_t compressedSize = sourceStream.Read<uint32_t>(std::endian::big);
            uint32_t decompressedSize = sourceStream.Read<uint32_t>(std::endian::big);
            std::span<const std::byte> checksum = sourceStream.Data().subspan(0, 16);
            sourceStream.SkipRead(16);

            engine.UpdateData(utility::byteswap(compressedSize));
            engine.UpdateData(utility::byteswap(decompressedSize));
            engine.UpdateData(checksum);

            chunkHandlers.emplace_back([&](IReadableStream& source, IWritableStream& target) {
                io::SpanStream chunkStream { source.Data().subspan(0, compressedSize) };
                source.SkipRead(compressedSize);

                uint8_t compressionMode = chunkStream.Read<uint8_t>();
                switch (compressionMode) {
                    case 'N':
                    {
                        std::size_t writtenBytes = target.Write(chunkStream.Data(), std::endian::little);
                        return writtenBytes == decompressedSize
                            ? errors::Success
                            : blte::ChunkDecompressionFailure(encodingKey, i, sourceStream.GetReadCursor());
                    }
                    case 'Z':
                    {
                        z_stream strm{
                            .next_in = 0,
                            .avail_in = 0,
                            .total_in = 0,
                            .next_out = Z_NULL,
                            .avail_out = 0,
                            .total_out = 0,
                            .zalloc = Z_NULL,
                            .zfree = Z_NULL,
                            .opaque = Z_NULL,
                        };
                        int ret = inflateInit(&strm);
                        if (ret != Z_OK)
                            return blte::ChunkDecompressionFailure(encodingKey, i, sourceStream.GetReadCursor());

                        std::unique_ptr<z_stream, decltype(&inflateEnd)> raii(std::addressof(strm), &inflateEnd);

                        strm.avail_in = chunkStream.Data().size();
                        strm.next_in = const_cast<uint8_t*>(chunkStream.Data<uint8_t>().data());

                        std::array<uint8_t, 8192> decompressedBuffer;
                        while (ret != Z_STREAM_END && strm.avail_in != 0) {
                            strm.avail_out = decompressedBuffer.size();
                            strm.next_out = decompressedBuffer.data();

                            ret = inflate(&strm, Z_NO_FLUSH);
                            if (ret < 0)
                                return blte::ChunkDecompressionFailure(encodingKey, i, sourceStream.GetReadCursor());

                            std::size_t writeCount = target.Write(std::span{ decompressedBuffer.data(), decompressedBuffer.size() - strm.avail_out }, std::endian::little);

                            // Should this error be more specific?
                            if (writeCount != decompressedBuffer.size() - strm.avail_out)
                                return blte::ChunkDecompressionFailure(encodingKey, i, sourceStream.GetReadCursor());
                        }

                        return errors::Success;
                    }
                    case 'F':
                        return operator() (source, target, encodingKey, contentKey, false);
                    default:
                        return blte::UnsupportedCompressionMode(encodingKey, i, compressionMode, sourceStream.GetReadCursor());
                }
            });
        }

        engine.Finalize();
        crypto::MD5::Digest encodingDigest = engine.GetDigest();

        if (validateKeys && encodingKey != nullptr && *encodingKey != encodingDigest)
            return blte::InvalidSignature(encodingKey);

        for (auto&& handler : chunkHandlers) {
            Error error = handler(sourceStream, targetStream);
            if (error != errors::Success)
                return error;
        }

        return errors::Success;
    }

    Result<io::GrowableMemoryStream> BlockTableEncodedStreamTransform::operator () (IReadableStream& sourceStream, libtactmon::tact::EKey const* encodingKey, libtactmon::tact::CKey const* contentKey) const noexcept {
        io::GrowableMemoryStream target;

        Error error = operator () (sourceStream, target, encodingKey, contentKey, encodingKey != nullptr && contentKey != nullptr);

        return error == errors::Success
            ? Result<io::GrowableMemoryStream> { std::move(target) }
            : Result<io::GrowableMemoryStream> { std::move(error) };
    }
}

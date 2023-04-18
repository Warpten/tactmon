#include "libtactmon/io/IStream.hpp"
#include "libtactmon/tact/BLTE.hpp"
#include "libtactmon/crypto/Hash.hpp"
#include "libtactmon/utility/Endian.hpp"

#include <bit>
#include <cstdint>
#include <vector>

#include <zlib.h>

#include <spdlog/spdlog.h>

struct ChunkHeader {
    std::size_t CompressedSize = 0;
    std::size_t DecompressedSize = 0;
    std::array<uint8_t, 16> Checksum;

    std::size_t Offset = 0; // Calculated
};

namespace libtactmon::tact {
    std::optional<BLTE> BLTE::Parse(io::IReadableStream& fstream) {
        return _Parse(fstream, nullptr, nullptr);
    }
    std::optional<BLTE> BLTE::Parse(io::IReadableStream& fstream, tact::EKey const& ekey, tact::CKey const& ckey) {
        return _Parse(fstream, &ekey, &ckey);
    }

    std::optional<BLTE> BLTE::_Parse(io::IReadableStream& fstream, tact::EKey const* ekey, tact::CKey const* ckey) {
        uint32_t magic = fstream.Read<uint32_t>(std::endian::big);
        if (magic != 'BLTE')
            return std::nullopt;

        uint32_t headerSize = fstream.Read<uint32_t>(std::endian::big);
        uint32_t flagsChunkCount = fstream.Read<uint32_t>(std::endian::big);

        // Validate EKey
        crypto::MD5 engine;
        engine.UpdateData(utility::byteswap(magic));
        engine.UpdateData(utility::byteswap(headerSize));
        engine.UpdateData(utility::byteswap(flagsChunkCount));

        uint8_t flags = (flagsChunkCount & 0xFF000000) >> 24;
        uint32_t chunkCount = flagsChunkCount & 0x00FFFFFF;

        std::unique_ptr<ChunkHeader[]> chunks = std::make_unique<ChunkHeader[]>(chunkCount);
        
        for (std::size_t i = 0; i < chunkCount; ++i) {
            chunks[i].CompressedSize = fstream.Read<uint32_t>(std::endian::big);
            chunks[i].DecompressedSize = fstream.Read<uint32_t>(std::endian::big);
            fstream.Read(chunks[i].Checksum, std::endian::little);

            engine.UpdateData(utility::byteswap(chunks[i].CompressedSize));
            engine.UpdateData(utility::byteswap(chunks[i].DecompressedSize));
            engine.UpdateData(chunks[i].Checksum);

            chunks[i].Offset = i == 0
                ? headerSize 
                : (chunks[i - 1].Offset + chunks[i - 1].CompressedSize);
        }

        engine.Finalize();
        crypto::MD5::Digest checksum = engine.GetDigest();

        if (ekey != nullptr && *ekey != checksum) {
            spdlog::critical("Validation of BLTE archive {} failed: EKey key does not match header checksum.", ekey->ToString());

            return std::nullopt;
        }
        
        BLTE blte { };
        for (std::size_t i = 0; i < chunkCount; ++i) {
            fstream.SeekRead(chunks[i].Offset);
            if (!blte.LoadChunk(fstream, chunks[i].CompressedSize, chunks[i].DecompressedSize, chunks[i].Checksum)) {
                if (ekey != nullptr)
                    spdlog::critical("Failed to read a chunk from BLTE archive {}: checksum mismatch.", ekey->ToString());

                return std::nullopt;
            }
        }

        if (ckey != nullptr && !blte.Validate(*ckey)) {
            if (ekey != nullptr)
                spdlog::critical("Validation of BLTE archive {} failed: CKey does not match contents checksum.", ekey->ToString());

            return std::nullopt;
        }

        return blte;
    }

    BLTE::BLTE() = default;

    bool BLTE::LoadChunk(io::IReadableStream& stream, std::size_t compressedSize, std::size_t decompressedSize, std::array<uint8_t, 16> checksum) {
        if (stream.GetReadCursor() + compressedSize > stream.GetLength())
            return false;

        std::span<const uint8_t> chunkSpan = stream.Data<uint8_t>().subspan(0, compressedSize);

        // Ensure data matches checksum
        crypto::MD5::Digest digest = crypto::MD5::Of(chunkSpan);
        if (!std::equal(digest.begin(), digest.end(), checksum.begin(), checksum.end()))
            return false;

        // Load data into stream
        switch (chunkSpan[0]) {
            case 'N':
            {
                std::size_t writeCount = _dataBuffer.Write(chunkSpan.subspan(1), std::endian::little);
                return writeCount == chunkSpan.size() - 1;
            }
            case 'Z':
            {
                z_stream strm {
                    .next_in   = 0,
                    .avail_in  = 0,
                    .total_in  = 0,
                    .next_out  = Z_NULL,
                    .avail_out = 0,
                    .total_out = 0,
                    .zalloc    = Z_NULL,
                    .zfree     = Z_NULL,
                    .opaque    = Z_NULL,
                };
                int ret = inflateInit(&strm);
                if (ret != Z_OK)
                    return false;

                strm.avail_in = chunkSpan.subspan(1).size();
                strm.next_in = const_cast<uint8_t*>(chunkSpan.subspan(1).data());

                std::array<uint8_t, 8192> decompressedBuffer;
                while (ret != Z_STREAM_END && strm.avail_in != 0) {
                    strm.avail_out = decompressedBuffer.size();
                    strm.next_out = decompressedBuffer.data();

                    ret = inflate(&strm, Z_NO_FLUSH);
                    if (ret < 0) {
                        ret = inflateEnd(&strm);
                        return false;
                    }

                    std::size_t writeCount = _dataBuffer.Write(std::span { decompressedBuffer.data(), decompressedBuffer.size() - strm.avail_out }, std::endian::little);
                    if (writeCount != decompressedBuffer.size() - strm.avail_out)
                        return false;
                }

                ret = inflateEnd(&strm);
                return true;
            }
            case 'F':
            {
                // Recursive BLTE file. Not implemented.
                return false;
            }
            default:
                spdlog::critical("Encountered unsupported encoding mode {} in BLTE archive. See additional information below.", char(chunkSpan[0]));
                return false;
        }
    }

    bool BLTE::Validate(tact::CKey const& ckey) const {
        crypto::MD5::Digest checksum = crypto::MD5::Of(_dataBuffer.Data());

        return ckey == checksum;
    }
}

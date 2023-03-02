#include "beast/BlockTableEncodedStreamTransform.hpp"

#include <boost/beast/http.hpp>

#include <zlib.h>

namespace boost::beast::user {
    BlockTableEncodedStreamTransform::BlockTableEncodedStreamTransform(OutputHandler handler, InputFeedback feedback) 
        : _handler(handler), _feedback(feedback), _ms(std::endian::little)
    { }

    std::size_t BlockTableEncodedStreamTransform::Parse(uint8_t* data, size_t size, boost::beast::error_code& ec) {
        _ms.Write(std::span<uint8_t> { data, size }, std::endian::little);
        _feedback(size);

        for (;;) {
            switch (_step) {
                case Step::Header:
                {
                    if (!_ms.CanRead(8))
                        return size;

                    uint32_t signature = _ms.Read<uint32_t>(std::endian::little);
                    if (signature != 0x45544c42) {
                        ec = boost::beast::http::error::body_limit;
                        return size;
                    }

                    _headerSize = _ms.Read<uint32_t>(std::endian::big);

                    _step = Step::ChunkInfo;
                    break;
                }
                case Step::ChunkInfo:
                {
                    if (!_ms.CanRead(_headerSize))
                        return size;

                    uint32_t flagsChunkCount = _ms.Read<uint32_t>(std::endian::big);

                    uint8_t flags = (flagsChunkCount & 0xFF000000) >> 24;
                    uint32_t chunkCount = flagsChunkCount & 0x00FFFFFF;
                    if (flags != 0xF || chunkCount == 0) {
                        ec = boost::beast::http::error::need_more;
                        return size;
                    }

                    _chunks.resize(chunkCount);

                    for (size_t i = 0; i < chunkCount; ++i) {
                        _chunks[i].compressedSize = _ms.Read<uint32_t>(std::endian::big) - 1;
                        _chunks[i].decompressedSize = _ms.Read<uint32_t>(std::endian::big);
                        _ms.Read(_chunks[i].checksum, std::endian::little);
                    }

                    _step = Step::DataBlocks;
                    break;
                }
                default:
                {
                    if (_step - Step::DataBlocks >= _chunks.size())
                        return size;

                    ChunkInfo& chunkInfo = _chunks[_step - Step::DataBlocks];
                    if (!_ms.CanRead(chunkInfo.compressedSize))
                        return size;

                    uint8_t encodingMode = _ms.Read<uint8_t>(std::endian::big);
                    switch (encodingMode)
                    {
                        case 'N':
                        {
                            // Using C style cast because cba const_cast(reinterpret_cast())
                            _handler(std::span<uint8_t const> { reinterpret_cast<uint8_t const*>(_ms.Data()), chunkInfo.compressedSize });
                            _ms.SkipRead(chunkInfo.compressedSize);
                            break;
                        }
                        case 'Z':
                        {
                            z_stream strm;
                            strm.zalloc = Z_NULL;
                            strm.zfree = Z_NULL;
                            strm.opaque = Z_NULL;
                            strm.avail_in = 0;
                            strm.next_in = Z_NULL;
                            int ret = inflateInit(&strm);
                            if (ret != Z_OK)
                            {
                                ec = boost::beast::http::error::bad_alloc;
                                return size;
                            }

                            strm.avail_in = chunkInfo.compressedSize;
                            strm.next_in = (uint8_t*) (_ms.Data());

                            std::array<uint8_t, 8192> decompressedBuffer;
                            while (ret != Z_STREAM_END && strm.avail_in != 0) {
                                strm.avail_out = decompressedBuffer.size();
                                strm.next_out = decompressedBuffer.data();

                                ret = inflate(&strm, Z_NO_FLUSH);
                                if (ret < 0) {
                                    ret = inflateEnd(&strm);

                                    ec = boost::beast::http::error::bad_alloc;
                                    return size;
                                }

                                _handler(std::span<uint8_t const> { decompressedBuffer.data(), decompressedBuffer.size() - strm.avail_out });
                            }

                            ret = inflateEnd(&strm);
                            _ms.SkipRead(chunkInfo.compressedSize);
                            break;
                        }
                        case 'F':
                        {
                            // Nested BLTE stream
                            BlockTableEncodedStreamTransform nestedReader { _handler };
                            nestedReader.Parse((uint8_t*) _ms.Data(), chunkInfo.compressedSize, ec);
                            _ms.SkipRead(chunkInfo.compressedSize);
                            break;
                        }
                        default:
                        {
                            // Not implemented - set error_code
                            ec = boost::beast::http::error::unexpected_body;
                            return size;
                        }
                    }

                    _step = static_cast<Step>(static_cast<uint32_t>(_step) + 1);
                    break;
                }
            }
        }

        return size;
    }
}

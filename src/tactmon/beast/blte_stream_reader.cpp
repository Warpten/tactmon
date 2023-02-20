#include "beast/blte_stream_reader.hpp"

#include <boost/beast/http.hpp>

#include <zlib.h>

namespace boost::beast::user {
    blte_stream_reader::blte_stream_reader() : _ms(std::endian::little) { }

    std::size_t blte_stream_reader::write_some(uint8_t* data, size_t size, std::function<void(uint8_t*, size_t)> acceptor, boost::beast::error_code& ec) {
        _ms.Write(std::span<uint8_t> { data, size }, std::endian::little);

        while (true) {
            switch (_step) {
                case step::header:
                {
                    if (!_ms.CanRead(8))
                        return size;

                    uint32_t signature = _ms.Read<uint32_t>(std::endian::little);
                    if (signature != 0x45544c42) {
                        ec = boost::beast::http::error::need_more;
                        return size;
                    }

                    _headerSize = _ms.Read<uint32_t>(std::endian::big);

                    _step = step::chunk_info;
                    break;
                }
                case step::chunk_info:
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
                        _chunks[i].compressedSize = _ms.Read<uint32_t>(std::endian::big);
                        _chunks[i].decompressedSize = _ms.Read<uint32_t>(std::endian::big);
                        _ms.Read(_chunks[i].checksum, std::endian::little);
                    }

                    _step = step::data_blocks;
                    break;
                }
                default:
                {
                    if (_step - step::data_blocks >= _chunks.size()) {
                        return size;
                    }

                    chunk_info_t& chunkInfo = _chunks[_step - step::data_blocks];
                    if (!_ms.CanRead(chunkInfo.compressedSize))
                        return size;

                    uint8_t encodingMode = _ms.Read<uint8_t>(std::endian::big);
                    switch (encodingMode)
                    {
                        case 'N':
                        {
                            // Using C style cast because cba const_cast(reinterpret_cast())
                            acceptor((uint8_t*) (_ms.Data()), chunkInfo.compressedSize - 1);
                            _ms.SkipRead(chunkInfo.compressedSize - 1);
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

                            strm.avail_in = chunkInfo.compressedSize - 1;
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

                                acceptor(decompressedBuffer.data(), decompressedBuffer.size() - strm.avail_out);
                            }

                            ret = inflateEnd(&strm);
                            _ms.SkipRead(chunkInfo.compressedSize - 1);
                            break;
                        }
                        default:
                        {
                            // Not implemented - set error_code
                            ec = boost::beast::http::error::unexpected_body;
                            return size;
                        }
                    }

                    _step = static_cast<step>(static_cast<uint32_t>(_step) + 1);
                    break;
                }
            }
        }

        return size;
    }
}

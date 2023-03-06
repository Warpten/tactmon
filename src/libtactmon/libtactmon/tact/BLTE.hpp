#pragma once

#include <array>
#include <cstdint>
#include <optional>

#include "libtactmon/io/MemoryStream.hpp"
#include "libtactmon/tact/CKey.hpp"
#include "libtactmon/tact/EKey.hpp"

namespace libtactmon::tact {
    struct BLTE final {
        /**
         * Parses a BLTE archive from the given stream, validating its contents against the ekey and ckey provided, and returning
         * the decompressed data stream if successful.
         * 
         * @param[in] fstream An input stream.
         * @param[in] ekey    An encoding key.
         * @param[in] ckey    A content key.
         * 
         * @returns The decompressed data stream, or an empty optional if decompression was unsuccessful.
         * 
         * @remarks This function is not lazy; the contents of the decompressed file are loaded to memory.
         */
        static std::optional<BLTE> Parse(io::IReadableStream& fstream, tact::EKey const& ekey, tact::CKey const& ckey);


        /**
         * Parses a BLTE archive from the given stream, and returning the decompressed data stream if successful.
         *
         * @param[in] fstream An input stream.
         *
         * @returns The decompressed data stream, or an empty optional if decompression was unsuccessful.
         *
         * @remarks This function is not lazy; the contents of the decompressed file are loaded to memory.
         */
        static std::optional<BLTE> Parse(io::IReadableStream& fstream);

    private:
        static std::optional<BLTE> _Parse(io::IReadableStream& fstream, tact::EKey const* ekey, tact::CKey const* ckey);

        explicit BLTE();
        bool LoadChunk(io::IReadableStream& stream, size_t compressedSize, size_t decompressedSize, std::array<uint8_t, 16> checksum);
        bool Validate(tact::CKey const& ckey) const;

    public:
        io::IReadableStream& GetStream() { return _dataBuffer; }

    private:
        io::GrowableMemoryStream _dataBuffer;
    };
}

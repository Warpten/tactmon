#pragma once

#include <array>
#include <cstdint>
#include <optional>

#include "io/mem/MemoryStream.hpp"
#include "tact/CKey.hpp"
#include "tact/EKey.hpp"

namespace tact {
    struct BLTE final {
        static std::optional<BLTE> Parse(io::IReadableStream& fstream, tact::EKey const& ekey, tact::CKey const& ckey);
        static std::optional<BLTE> Parse(io::IReadableStream& fstream);

    private:
        static std::optional<BLTE> _Parse(io::IReadableStream& fstream, tact::EKey const* ekey, tact::CKey const* ckey);

        explicit BLTE();
        bool LoadChunk(io::IReadableStream& stream, size_t compressedSize, size_t decompressedSize, std::array<uint8_t, 16> checksum);
        bool Validate(tact::CKey const& ckey) const;

    public:
        io::IReadableStream& GetStream() { return _dataBuffer; }

    private:
        io::mem::GrowableMemoryStream _dataBuffer{ std::endian::little };
    };
}

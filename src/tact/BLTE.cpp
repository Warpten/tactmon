#include "io/IStream.hpp"
#include "tact/BLTE.hpp"
#include "crypto/Hash.hpp"
#include "util/Endian.hpp"

#include <bit>
#include <cstdint>
#include <vector>

namespace tact {
	std::optional<BLTE> BLTE::Parse(io::IReadableStream& fstream, tact::EKey const& ekey) {
		uint32_t magic = fstream.Read<uint32_t>(std::endian::big);
		if (magic != 'BLTE')
			return std::nullopt;

		uint32_t headerSize = fstream.Read<uint32_t>(std::endian::big);
		uint32_t flagsChunkCount = fstream.Read<uint32_t>(std::endian::big);

		// Validate EKey
		crypto::MD5 engine;
		engine.UpdateData(util::byteswap(magic));
		engine.UpdateData(util::byteswap(headerSize));
		engine.UpdateData(util::byteswap(flagsChunkCount));

		uint8_t flags = (flagsChunkCount & 0xFF000000) >> 24;
		uint32_t chunkCount = flagsChunkCount & 0x00FFFFFF;

		for (size_t i = 0; i < chunkCount; ++i) {
			uint32_t compressedSize = fstream.Read<uint32_t>(std::endian::big);
			uint32_t decompressedSize = fstream.Read<uint32_t>(std::endian::big);

			std::array<uint8_t, 16> checksum;
			fstream.Read(checksum, std::endian::little);

			engine.UpdateData(util::byteswap(compressedSize));
			engine.UpdateData(util::byteswap(decompressedSize));
			engine.UpdateData(checksum);
		}

		engine.Finalize();
		crypto::MD5::Digest checksum = engine.GetDigest();

		if (!std::equal(ekey.Value().begin(), ekey.Value().end(), checksum.begin(), checksum.end()))
			return std::nullopt;

		fstream.SeekRead(sizeof(magic));
		return BLTE { fstream };
	}

	BLTE::BLTE(io::IReadableStream& fstream) {
		uint32_t headerSize = fstream.Read<uint32_t>(std::endian::big);

		if (headerSize == 0) {
			// Implement (the rest of the file is a data chunk)
		} else {
			uint32_t flagsChunkCount = fstream.Read<uint32_t>(std::endian::big);

			uint8_t flags = (flagsChunkCount & 0xFF000000) >> 24;
			uint32_t chunkCount = flagsChunkCount & 0x00FFFFFF;

			for (size_t i = 0; i < chunkCount; ++i) {
				uint32_t compressedSize = fstream.Read<uint32_t>(std::endian::big);
				uint32_t decompressedSize = fstream.Read<uint32_t>(std::endian::big);

				std::array<uint8_t, 16> checksum;
				fstream.Read(checksum, std::endian::big);
			}
		}
	}
}

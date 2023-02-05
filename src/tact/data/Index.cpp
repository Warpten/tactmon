#include "tact/data/Index.hpp"
#include "io/IStream.hpp"
#include "crypto/Hash.hpp"

#include <boost/algorithm/hex.hpp>

namespace tact::data {
    Index::Index(std::string_view hash, io::IReadableStream& stream)
        : _archiveName(hash)
    {
        std::vector<uint8_t> hashBytes;
        boost::algorithm::unhex(hash.begin(), hash.end(), std::back_inserter(hashBytes));

        size_t checksumSize = 0x10;
        while (checksumSize > 0) {
            size_t footerSize = checksumSize * 2 + sizeof(uint8_t) * 8 + sizeof(uint32_t);

            stream.SeekRead(stream.GetLength() - footerSize);
            std::span<uint8_t> footerData { (uint8_t*) stream.Data(), footerSize };
            auto digest = crypto::MD5::Of(footerData);

            if (std::equal(hashBytes.begin(), hashBytes.end(), digest.begin(), digest.end()))
                break;

            --checksumSize;
        }

        if (checksumSize == 0)
            return;

        size_t footerSize = checksumSize * 2 + sizeof(uint8_t) * 8 + sizeof(uint32_t);
        stream.SeekRead(stream.GetLength() - footerSize);

        // Read footer
        std::span<uint8_t> tocHash { (uint8_t*) stream.Data(), checksumSize };
        stream.SkipRead(checksumSize);

        uint8_t version = stream.Read<uint8_t>(std::endian::little);
        uint8_t _11 = stream.Read<uint8_t>(std::endian::little);
        uint8_t _12 = stream.Read<uint8_t>(std::endian::little);
        uint8_t blockSizeKb = stream.Read<uint8_t>(std::endian::little);
        uint8_t offsetBytes = stream.Read<uint8_t>(std::endian::little);
        uint8_t sizeBytes = stream.Read<uint8_t>(std::endian::little);
        _keySizeBytes = stream.Read<uint8_t>(std::endian::little);
        stream.Read<uint8_t>(std::endian::little); // checksumSize, validate!
        uint32_t numElements = stream.Read<uint32_t>(std::endian::little);
        // We don't read the footer checksum (but probably should)
        // > footerChecksum is calculated over the footer beginning with version when footerChecksum is zeroed
        //   ????

        // Compute some file properties
        size_t blockSize = blockSizeKb << 10;
        size_t entrySize = _keySizeBytes + sizeBytes + offsetBytes;
        size_t entryCount = blockSize / entrySize;
        size_t paddingSize = blockSize - entrySize * entryCount;

        // Compute block count. Integrate a block's data in the TOC to do the math, since there is only one
        // TOC entry per block (well, technically, two entries; one corresponding to the last EKey of a block,
        // and one corresponding to the lower part of the MD5 of a block)
        size_t blockCount = (stream.GetLength() - footerSize) / (blockSize + (_keySizeBytes + checksumSize));

        // Block data is stored flattened.
        // We collapse all the encoding keys in a single buffer, and just index into it when keying the file entries.

        // Reserve storage for the actual keys
        _keyBuffer.reserve(entryCount * _keySizeBytes * blockCount);

        for (size_t i = 0; i < blockCount; ++i) {
            stream.SeekRead(i * blockSize);
            std::span<uint8_t> rawBlockData { (uint8_t*) stream.Data(), blockSize };

            // Skip over TOC's first array, effectively getting to the block hash of this block
            stream.SeekRead(blockCount * blockSize + blockCount * _keySizeBytes + i * checksumSize);
            std::span<uint8_t> checksum { (uint8_t*) stream.Data(), checksumSize };
            crypto::MD5::Digest digest = crypto::MD5::Of(rawBlockData);
            if (!std::equal(digest.begin(), digest.begin() + checksumSize, checksum.begin(), checksum.end()))
                continue;

            for (size_t j = 0; j < entryCount; ++j) {
                stream.SeekRead(i * blockSize + j * entrySize);

                // Read the key and insert it into storage
                std::span<uint8_t> keyData{ (uint8_t*)stream.Data(), _keySizeBytes };
                stream.SkipRead(_keySizeBytes);

                if (std::ranges::all_of(keyData, [](uint8_t b){ return b == 0; }))
                    continue;

                size_t keyOfs = _keyBuffer.size();
                _keyBuffer.insert(_keyBuffer.end(), keyData.begin(), keyData.end());
                _entries.emplace_back(stream, sizeBytes, offsetBytes, keyOfs);
            }
        }

        _keyBuffer.shrink_to_fit();
    }

    Index::Entry::Entry(io::IReadableStream& stream, size_t sizeBytes, size_t offsetBytes, size_t keyOffset)
        : _keyOffset(keyOffset)
    {
        std::span<uint8_t> dataBytes { (uint8_t*) stream.Data(), sizeBytes + offsetBytes };
        stream.SkipRead(sizeBytes + offsetBytes);

        for (size_t i = 0; i < sizeBytes; ++i) {
            _size = (_size << 8) | dataBytes[i];
        }

        for (size_t i = 0; i < offsetBytes; ++i) {
            _offset = (_offset << 8) | dataBytes[sizeBytes + i];
        }
    }

    std::span<const uint8_t> Index::Entry::key(Index const& index) const {
        return std::span<const uint8_t> { index._keyBuffer.data() + _keyOffset, index._keySizeBytes };
    }

    auto Index::operator [] (tact::EKey const& key) const -> const Entry* {
        auto itr = std::find_if(_entries.begin(), _entries.end(), [this, needle = key.data()](Entry const& entry) {
            auto entryKey = entry.key(*this);
            return std::equal(entryKey.begin(), entryKey.end(), needle.begin(), needle.end());
        });
        if (itr != _entries.end())
            return std::addressof(*itr);

        return nullptr;
    }
}

#include "libtactmon/tact/data/Index.hpp"
#include "libtactmon/io/IReadableStream.hpp"
#include "libtactmon/crypto/Hash.hpp"

#include <boost/algorithm/hex.hpp>

namespace libtactmon::tact::data {
    Index::Index(std::string_view hash, io::IReadableStream& stream)
        : _archiveName(hash), _keySizeBytes(0)
    {
        std::vector<uint8_t> hashBytes;
        boost::algorithm::unhex(hash.begin(), hash.end(), std::back_inserter(hashBytes));

        std::size_t checksumSize = 0x10;
        while (checksumSize > 0) {
            std::size_t footerSize = checksumSize * 2 + sizeof(uint8_t) * 8 + sizeof(uint32_t);

            stream.SeekRead(stream.GetLength() - footerSize);
            if (!stream.CanRead(footerSize)) // Protect against underflow
                continue;

            std::span<const uint8_t> footerData = stream.Data<uint8_t>().subspan(0, footerSize);
            auto digest = crypto::MD5::Of(footerData);

            if (std::equal(hashBytes.begin(), hashBytes.end(), digest.begin(), digest.end()))
                break;

            --checksumSize;
        }

        // Unable to validate the file, exit out
        if (checksumSize == 0)
            return;

        std::size_t footerSize = checksumSize * 2 + sizeof(uint8_t) * 8 + sizeof(uint32_t);
        stream.SeekRead(stream.GetLength() - footerSize);

        // Read footer
        std::span<const uint8_t> tocHash = stream.Data<uint8_t>().subspan(0, checksumSize);
        stream.SkipRead(checksumSize);

        uint8_t version = stream.Read<uint8_t>();
        uint8_t _11 = stream.Read<uint8_t>();
        uint8_t _12 = stream.Read<uint8_t>();
        uint8_t blockSizeKb = stream.Read<uint8_t>();
        uint8_t offsetBytes = stream.Read<uint8_t>();
        uint8_t sizeBytes = stream.Read<uint8_t>();
        _keySizeBytes = stream.Read<uint8_t>();
        stream.Read<uint8_t>(); // checksumSize, validate!
        uint32_t numElements = stream.Read<uint32_t>();
        // We don't read the footer checksum (but probably should)
        // > footerChecksum is calculated over the footer beginning with version when footerChecksum is zeroed
        //   ????

        // Compute some file properties
        std::size_t blockSize = 1024uLL * blockSizeKb;
        std::size_t entrySize = _keySizeBytes + sizeBytes + offsetBytes;
        std::size_t entryCount = blockSize / entrySize;
        std::size_t paddingSize = blockSize - entrySize * entryCount;

        // Compute block count. Integrate a block's data in the TOC to do the math, since there is only one
        // TOC entry per block (well, technically, two entries; one corresponding to the last EKey of a block,
        // and one corresponding to the lower part of the MD5 of a block)
        std::size_t blockCount = (stream.GetLength() - footerSize) / (blockSize + (_keySizeBytes + checksumSize));

        // Block data is stored flattened.
        // We collapse all the encoding keys in a single buffer, and just index into it when keying the file entries.

        // Reserve storage for the actual keys
        _keyBuffer.reserve(entryCount * _keySizeBytes * blockCount);

        for (std::size_t i = 0; i < blockCount; ++i) {
            stream.SeekRead(i * blockSize);
            std::span<const uint8_t> rawBlockData = stream.Data<uint8_t>().subspan(0, blockSize);

            // Skip over TOC's first array, effectively getting to the block hash of this block
            stream.SeekRead(blockCount * blockSize + blockCount * _keySizeBytes + i * checksumSize);
            std::span<const uint8_t> checksum = stream.Data<uint8_t>().subspan(0, checksumSize);
            crypto::MD5::Digest digest = crypto::MD5::Of(rawBlockData);
            if (!std::equal(digest.begin(), digest.begin() + checksumSize, checksum.begin(), checksum.end()))
                continue;

            for (std::size_t j = 0; j < entryCount; ++j) {
                stream.SeekRead(i * blockSize + j * entrySize);

                // Read the key and insert it into storage
                std::span<const uint8_t> keyData = stream.Data<uint8_t>().subspan(0, _keySizeBytes);
                stream.SkipRead(_keySizeBytes);

                // If the key is all 0s, that's an end marker
                if (std::ranges::all_of(keyData, [](uint8_t b){ return b == 0; }))
                    continue;

                std::size_t keyOfs = _keyBuffer.size();
                _keyBuffer.insert(_keyBuffer.end(), keyData.begin(), keyData.end());
                _entries.emplace_back(stream, sizeBytes, offsetBytes, keyOfs);
            }
        }

        _keyBuffer.shrink_to_fit();
    }

    Index::Entry::Entry(io::IReadableStream& stream, std::size_t sizeBytes, std::size_t offsetBytes, std::size_t keyOffset)
        : _keyOffset(keyOffset)
    {
        std::span<const uint8_t> dataBytes = stream.Data<uint8_t>().subspan(0, sizeBytes + offsetBytes);
        stream.SkipRead(sizeBytes + offsetBytes);

        for (std::size_t i = 0; i < sizeBytes; ++i) {
            _size = (_size << 8) | dataBytes[i];
        }

        for (std::size_t i = 0; i < offsetBytes; ++i) {
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

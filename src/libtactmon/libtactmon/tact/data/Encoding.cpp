#include "libtactmon/io/IReadableStream.hpp"
#include "libtactmon/tact/data/Encoding.hpp"

#include <array>
#include <memory>

#include <assert.hpp>

namespace libtactmon::tact::data {
    uint64_t ReadUInt40(io::IReadableStream& stream, std::endian endianness) {
        std::array<uint8_t, 5> bytes;
        stream.Read(std::span<uint8_t> { bytes.data(), 5 }, endianness);

        uint64_t value = 0;
        for (uint8_t byte : bytes)
            value = (value << 8) | byte;
        return value;
    }

    Encoding::Header::Header(io::IReadableStream& stream) {
        if (!stream.CanRead(2 + 1 + 1 + 1 + 2 + 2 + 4 + 4 + 1 + 4))
            return;

        Signature          = stream.Read<uint16_t>(std::endian::big);
        Version            = stream.Read<uint8_t>();
        EncodingKeySize    = stream.Read<uint8_t>();
        ContentKeySize     = stream.Read<uint8_t>();
        CEKey.PageSize     = stream.Read<uint16_t>(std::endian::big) * 1024;
        EKeySpec.PageSize  = stream.Read<uint16_t>(std::endian::big) * 1024;
        CEKey.PageCount    = stream.Read<uint32_t>(std::endian::big);
        EKeySpec.PageCount = stream.Read<uint32_t>(std::endian::big);
        uint8_t unknown    = stream.Read<uint8_t>(); // Asserted to be zero by agent.
        DEBUG_ASSERT(unknown == 0, "Encoding header unknown has to be zero");
        ESpecBlockSize     = stream.Read<uint32_t>(std::endian::big);
    }

    Encoding::CEKeyPageTable::CEKeyPageTable(io::IReadableStream& stream, Header const& header)
    {
        _keyCount = stream.Read<uint8_t>();
        _fileSize = ReadUInt40(stream, std::endian::big);

        _ckey.resize(header.ContentKeySize);
        stream.Read(std::span { _ckey }, std::endian::little);

        if (_keyCount * header.EncodingKeySize > 0) {
            _ekeys.resize(header.EncodingKeySize * _keyCount);
            stream.Read(std::span { _ekeys }, std::endian::little);
        }
    }

    Encoding::CEKeyPageTable::CEKeyPageTable(CEKeyPageTable&& other) noexcept : _fileSize(other._fileSize),
        _ckey(std::move(other._ckey)), _ekeys(std::move(other._ekeys)), _keyCount(other._keyCount)
    {
        other._keyCount = 0;
        other._fileSize = 0;
    }

    Encoding::CEKeyPageTable& Encoding::CEKeyPageTable::operator = (Encoding::CEKeyPageTable&& other) noexcept {
        _fileSize = other._fileSize;
        _ckey = std::move(other._ckey);
        _ekeys = std::move(other._ekeys);
        _keyCount = other._keyCount;

        other._keyCount = 0;
        other._fileSize = 0;

        return *this;
    }

    Encoding::CEKeyPageTable::operator bool() const {
        return !_ckey.empty() && !_ekeys.empty();
    }

    /* static */ std::size_t Encoding::CEKeyPageTable::HashSize(Header const& header) {
        return header.ContentKeySize;
    }

    tact::EKey Encoding::CEKeyPageTable::ekey(std::size_t index, Encoding const& owner) const {
        return tact::EKey { std::span<const uint8_t> { _ekeys.data() + index * owner._header.EncodingKeySize, owner._header.EncodingKeySize } };
    }
    tact::CKey Encoding::CEKeyPageTable::ckey(Encoding const& owner) const {
        return tact::CKey { std::span<const uint8_t> { _ckey.data(), owner._header.ContentKeySize } };
    }

    // ^^^ CEKeyPageTable / EKeySpecPageTable vvv

    Encoding::EKeySpecPageTable::EKeySpecPageTable(EKeySpecPageTable&& other) noexcept 
        : _ekey(std::move(other._ekey)), _especIndex(other._especIndex), _fileSize(other._fileSize)
    {
        other._especIndex = 0;
        other._fileSize = 0;
    }

    Encoding::EKeySpecPageTable& Encoding::EKeySpecPageTable::operator = (Encoding::EKeySpecPageTable&& other) noexcept {
        _especIndex = other._especIndex;
        _fileSize = other._fileSize;
        _ekey = std::move(other._ekey);

        other._especIndex = 0;
        other._fileSize = 0;
        return *this;
    }

    Encoding::EKeySpecPageTable::operator bool() const {
        return !_ekey.empty();
    }

    Encoding::EKeySpecPageTable::EKeySpecPageTable(io::IReadableStream& stream, Header const& header) {
        std::size_t encodingKeySize = HashSize(header);
        if (!stream.CanRead(encodingKeySize + 4 + 5))
            return;

        _ekey.resize(encodingKeySize);
        stream.Read(std::span { _ekey.data(), encodingKeySize }, std::endian::little);

        _especIndex = stream.Read<uint32_t>(std::endian::big);
        _fileSize = ReadUInt40(stream, std::endian::big);
    }

    /* static */ std::size_t Encoding::EKeySpecPageTable::HashSize(Header const& header) {
        return header.EncodingKeySize;
    }

    // ^^^ EKeySpecPageTable / Encoding vvv

    Encoding::Encoding(io::IReadableStream& stream) : _header{ stream } {
        if (_header.Signature != 0x454E)
            return;

        stream.SkipRead(_header.ESpecBlockSize); // Skip ESpec strings

        std::size_t pageStart = stream.GetReadCursor() + static_cast<std::size_t>(_header.CEKey.PageCount) * (0x10uLL + _header.EncodingKeySize);

        _cekeyPages.reserve(_header.CEKey.PageCount);

        for (std::size_t i = 0; i < _header.CEKey.PageCount; ++i) {
            std::size_t pageOffset = pageStart + i * _header.CEKey.PageSize;
            std::size_t pageEnd = pageOffset + _header.CEKey.PageSize;

            _cekeyPages.emplace_back(stream, _header, pageOffset, pageEnd);
        }
    }

    Encoding::Encoding(Encoding&& other) noexcept
        : _header(other._header), _cekeyPages(std::move(other._cekeyPages)), _keySpecPageTables(std::move(other._keySpecPageTables))
    {
    }

    Encoding& Encoding::operator = (Encoding&& other) noexcept {
        _header = other._header;
        _cekeyPages = std::move(other._cekeyPages);
        _keySpecPageTables = std::move(other._keySpecPageTables);

        return *this;
    }

    Encoding::~Encoding() = default;

    std::size_t Encoding::count() const {
        std::size_t value = 0;
        for (const auto & ceKeyPage : _cekeyPages)
            value += ceKeyPage.size();
        return value;
    }

    std::size_t Encoding::GetContentKeySize() const {
        return _header.ContentKeySize;
    }

    std::optional<tact::data::FileLocation> Encoding::FindFile(tact::CKey const& contentKey) const {
        for (Page<CEKeyPageTable> const& page : _cekeyPages) {
            for (std::size_t j = 0; j < page.size(); ++j) {
                auto&& entry = page[j].ckey(*this);

                if (entry == contentKey)
                    return tact::data::FileLocation { page[j].fileSize(), page[j].keyCount(), std::span { page[j]._ekeys.data(), page[j].keyCount() * _header.EncodingKeySize } };
            }
        }

        return std::nullopt;
    }
}

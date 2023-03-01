
#include "libtactmon/io/IReadableStream.hpp"
#include "libtactmon/tact/data/Encoding.hpp"

#include <array>
#include <memory>

namespace libtactmon::tact::data {
    uint64_t ReadUInt40(io::IReadableStream& stream, std::endian endianness) {
        std::array<uint8_t, 5> bytes;
        stream.Read(std::span<uint8_t> { bytes.data(), 5 }, endianness);

        uint64_t value = 0;
        for (size_t i = 0; i < bytes.size(); ++i)
            value = (value << 8) | bytes[i];
        return value;
    }

    Encoding::Header::Header(io::IReadableStream& stream) {
        if (!stream.CanRead(2 + 1 + 1 + 1 + 2 + 2 + 4 + 4 + 1 + 4))
            return;

        Signature          = stream.Read<uint16_t>(std::endian::big);
        Version            = stream.Read<uint8_t>(std::endian::little);
        EncodingKeySize    = stream.Read<uint8_t>(std::endian::little);
        ContentKeySize     = stream.Read<uint8_t>(std::endian::little);
        CEKey.PageSize     = stream.Read<uint16_t>(std::endian::big) * 1024;
        EKeySpec.PageSize  = stream.Read<uint16_t>(std::endian::big) * 1024;
        CEKey.PageCount    = stream.Read<uint32_t>(std::endian::big);
        EKeySpec.PageCount = stream.Read<uint32_t>(std::endian::big);
        uint8_t unknown    = stream.Read<uint8_t>(std::endian::little); // Asserted to be zero by agent.
        ESpecBlockSize     = stream.Read<uint32_t>(std::endian::big);
    }

    Encoding::CEKeyPageTable::CEKeyPageTable(io::IReadableStream& stream, Header const& header)
    {
        _keyCount = stream.Read<uint8_t>(std::endian::little);
        _fileSize = ReadUInt40(stream, std::endian::big);

        _ckey = std::make_unique<uint8_t[]>(header.ContentKeySize);
        stream.Read(std::span { _ckey.get(), header.ContentKeySize }, std::endian::little);

        if (_keyCount * header.EncodingKeySize > 0) {
            _ekeys = std::make_unique<uint8_t[]>(header.EncodingKeySize * _keyCount);
            stream.Read(std::span{ _ekeys.get(), header.EncodingKeySize * _keyCount }, std::endian::little);
        }
    }

    Encoding::CEKeyPageTable::CEKeyPageTable(CEKeyPageTable&& other) noexcept 
        : _fileSize(other._fileSize),
        _ckey(std::move(other._ckey)), _ekeys(std::move(other._ekeys)), _keyCount(other._keyCount)
    { }

    Encoding::CEKeyPageTable& Encoding::CEKeyPageTable::operator = (Encoding::CEKeyPageTable&& other) noexcept {
        _fileSize = other._fileSize;
        _ckey = std::move(other._ckey);
        _ekeys = std::move(other._ekeys);
        _keyCount = other._keyCount;

        return *this;
    }

    Encoding::CEKeyPageTable::operator bool() const {
        return _ckey != nullptr && _ekeys != nullptr;
    }

    /* static */ size_t Encoding::CEKeyPageTable::HashSize(Header const& header) {
        return header.ContentKeySize;
    }

    tact::EKey Encoding::CEKeyPageTable::ekey(size_t index, Encoding const& owner) const {
        return { _ekeys.get() + index * owner._header.EncodingKeySize, owner._header.EncodingKeySize };
    }
    tact::CKey Encoding::CEKeyPageTable::ckey(Encoding const& owner) const {
        return { _ckey.get(), owner._header.ContentKeySize };
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
        return _ekey != nullptr;
    }

    Encoding::EKeySpecPageTable::EKeySpecPageTable(io::IReadableStream& stream, Header const& header) {
        size_t encodingKeySize = HashSize(header);
        if (!stream.CanRead(encodingKeySize + 4 + 5))
            return;

        _ekey = std::make_unique<uint8_t[]>(encodingKeySize);
        stream.Read(std::span{ _ekey.get(), encodingKeySize }, std::endian::little);

        _especIndex = stream.Read<uint32_t>(std::endian::big);
        _fileSize = ReadUInt40(stream, std::endian::big);
    }

    /* static */ size_t Encoding::EKeySpecPageTable::HashSize(Header const& header) {
        return header.EncodingKeySize;
    }

    // ^^^ EKeySpecPageTable / Encoding vvv

    Encoding::Encoding(io::IReadableStream& stream) : _header{ stream } {
        if (_header.Signature != 0x454E)
            return;

        stream.SkipRead(_header.ESpecBlockSize); // Skip ESpec strings

        size_t pageStart = stream.GetReadCursor() + _header.CEKey.PageCount * (_header.EncodingKeySize + 0x10);

        _cekeyPageCount = _header.CEKey.PageCount;
        _cekeyPages.reserve(_header.CEKey.PageCount);

        for (size_t i = 0; i < _header.CEKey.PageCount; ++i) {
            size_t pageOffset = pageStart + i * _header.CEKey.PageSize;
            size_t pageEnd = pageOffset + _header.CEKey.PageSize;

            _cekeyPages.emplace_back(stream, _header, pageOffset, pageEnd);
        }
    }

    Encoding::Encoding(Encoding&& other) noexcept
        : _header(std::move(other._header)), _cekeyPageCount(other._cekeyPageCount), _cekeyPages(std::move(other._cekeyPages)), _keySpecPageTables(std::move(other._keySpecPageTables))
    {
        other._cekeyPageCount = 0;
    }

    Encoding& Encoding::operator = (Encoding&& other) noexcept {
        _header = std::move(other._header);
        _cekeyPageCount = other._cekeyPageCount;
        _cekeyPages = std::move(other._cekeyPages);
        _keySpecPageTables = std::move(other._keySpecPageTables);

        other._cekeyPageCount = 0;

        return *this;
    }

    Encoding::~Encoding() {
        for (size_t i = 0; i < _cekeyPageCount; ++i)
            (&_cekeyPages[i])->~Page();
    }

    size_t Encoding::count() const {
        size_t value = 0;
        for (size_t i = 0; i < _cekeyPageCount; ++i)
            value += _cekeyPages[i].size();
        return value;
    }

    size_t Encoding::GetContentKeySize() const {
        return _header.ContentKeySize;
    }

    std::optional<tact::data::FileLocation> Encoding::FindFile(tact::CKey const& contentKey) const {
        for (size_t i = 0; i < _cekeyPageCount; ++i) {
            Page<CEKeyPageTable> const& page = _cekeyPages[i];

            for (size_t j = 0; j < page.size(); ++j) {
                auto&& entry = page[j].ckey(*this);

                if (entry == contentKey)
                    return tact::data::FileLocation { page[j].fileSize(), page[j].keyCount(), std::span<uint8_t> { page[j]._ekeys.get(), page[j].keyCount() * _header.EncodingKeySize }};
            }
        }

        return std::nullopt;
    }
}

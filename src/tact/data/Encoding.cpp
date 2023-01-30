#include "tact/data/Encoding.hpp"
#include "util/Endian.hpp"

#include <array>
#include <memory>

namespace tact::data {

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
        CEKey.HashSize     = stream.Read<uint8_t>(std::endian::little);
        EKeySpec.HashSize  = stream.Read<uint8_t>(std::endian::little);
        CEKey.PageSize     = stream.Read<uint16_t>(std::endian::big) * 1024;
        EKeySpec.PageSize  = stream.Read<uint16_t>(std::endian::big) * 1024;
        CEKey.PageCount    = stream.Read<uint32_t>(std::endian::big);
        EKeySpec.PageCount = stream.Read<uint32_t>(std::endian::big);
        uint8_t unknown    = stream.Read<uint8_t>(std::endian::little); // Asserted to be zero by agent.
        ESpecBlockSize     = stream.Read<uint32_t>(std::endian::big);
    }

    Encoding::CEKeyPageTable::CEKeyPageTable(io::IReadableStream& stream, Header const& header)
        : _ekeySize(header.CEKey.HashSize), _ckeySize(header.EKeySpec.HashSize)
    {
        _keyCount = stream.Read<uint8_t>(std::endian::little);
        _fileSize = ReadUInt40(stream, std::endian::big);

        _ckey = std::make_unique<uint8_t[]>(_ckeySize);
        stream.Read(std::span { _ckey.get(), _ckeySize }, std::endian::little);

        if (_keyCount * _ekeySize > 0) {
            _ekeys = std::make_unique<uint8_t[]>(_ekeySize * _keyCount);
            stream.Read(std::span{ _ekeys.get(), _ekeySize * _keyCount }, std::endian::little);
        }
    }

    Encoding::CEKeyPageTable::CEKeyPageTable(CEKeyPageTable&& other) noexcept 
        : _ekeySize(other._ekeySize), _ckeySize(other._ckeySize), _fileSize(other._fileSize),
        _ckey(std::move(other._ckey)), _ekeys(std::move(other._ekeys)), _keyCount(other._keyCount)
    { }

    Encoding::CEKeyPageTable& Encoding::CEKeyPageTable::operator = (Encoding::CEKeyPageTable&& other) noexcept {
        _ekeySize = other._ekeySize;
        _ckeySize = other._ckeySize;
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
        return header.CEKey.HashSize;
    }

    size_t Encoding::CEKeyPageTable::size() const {
        return _keyCount;
    }

    tact::EKey Encoding::CEKeyPageTable::operator [] (size_t index) const {
        return { _ekeys.get() + index * _ekeySize, _ekeySize };
    }

    size_t Encoding::CEKeyPageTable::fileSize() const { return _fileSize; }

    tact::CKey Encoding::CEKeyPageTable::ckey() const {
        return { _ckey.get(), _ckeySize };
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
        if (!stream.CanRead(header.EKeySpec.HashSize + 4 + 5))
            return;

        _ekey = std::make_unique<uint8_t[]>(header.EKeySpec.HashSize);
        stream.Read(std::span{ _ekey.get(), header.EKeySpec.HashSize }, std::endian::little);

        _especIndex = stream.Read<uint32_t>(std::endian::big);
        _fileSize = ReadUInt40(stream, std::endian::big);
    }

    /* static */ size_t Encoding::EKeySpecPageTable::HashSize(Header const& header) {
        return header.EKeySpec.HashSize;
    }

    // ^^^ EKeySpecPageTable / Encoding vvv

    Encoding::Encoding(io::IReadableStream& stream) : _header{ stream } {
        if (_header.Signature != 'EN')
            return;

        stream.SkipRead(_header.ESpecBlockSize); // Skip ESpec strings

        size_t pageStart = stream.GetReadCursor() + _header.CEKey.PageCount * (_header.CEKey.HashSize + 0x10);

        _cekeyPageCount = _header.CEKey.PageCount;
        _cekeyPages = std::make_unique<Page<CEKeyPageTable>[]>(_header.CEKey.PageCount);

        for (size_t i = 0; i < _header.CEKey.PageCount; ++i) {
            size_t pageOffset = pageStart + i * _header.CEKey.PageSize;
            size_t pageEnd = pageOffset + _header.CEKey.PageSize;

            new (&_cekeyPages[i]) Page<CEKeyPageTable>(stream, _header, pageOffset, pageEnd);
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
        return _header.CEKey.HashSize;
    }

    std::optional<tact::data::FileLocation> Encoding::FindFile(tact::CKey const& contentKey) const {
        for (size_t i = 0; i < _cekeyPageCount; ++i) {
            Page<CEKeyPageTable> const& page = _cekeyPages[i];

            for (size_t j = 0; j < page.size(); ++j) {
                auto&& entry = page[j].ckey();

                if (entry == contentKey)
                    return tact::data::FileLocation { page[j].fileSize(), page[j].size(), std::span<uint8_t> { page[j]._ekeys.get(), page[j]._ekeySize }};
            }
        }

        return std::nullopt;
    }
}

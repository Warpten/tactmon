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

    struct Header {
        uint16_t Signature;
        uint8_t Version;
        struct {
            uint32_t PageSize;
            uint32_t PageCount;
            uint8_t HashSize;
        } CEKey, EKeySpec;
        uint32_t ESpecBlockSize;

        Header(io::IReadableStream& stream) {
            Signature          = stream.Read<uint16_t>(std::endian::little);
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
    };

    Encoding::CEKeyPageTable::CEKeyPageTable(io::IReadableStream& stream, Header const& header)
        : _ekeySize(header.CEKey.HashSize), _ckeySize(header.EKeySpec.HashSize)
    {
        _keyCount = stream.Read<uint8_t>(std::endian::little);
        _fileSize = ReadUInt40(stream, std::endian::big);

        _ckey = std::make_unique<uint8_t[]>(_ckeySize);
        stream.Read(std::span { _ckey.get(), _ckeySize }, std::endian::little);
            
        _ekeys = std::make_unique<uint8_t[]>(_ekeySize * _keyCount);
        stream.Read(std::span { _ekeys.get(), _ekeySize * _keyCount }, std::endian::little);
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

    std::span<const uint8_t> Encoding::CEKeyPageTable::operator [] (size_t index) const {
        return std::span<const uint8_t> { _ekeys.get() + index * _ekeySize, _ekeySize };
    }

    size_t Encoding::CEKeyPageTable::fileSize() const { return _fileSize; }

    std::span<uint8_t const> Encoding::CEKeyPageTable::ckey() const {
        return std::span<const uint8_t> { _ckey.get(), _ckeySize };
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
        _ekey = std::make_unique<uint8_t[]>(header.EKeySpec.HashSize);
        stream.Read(std::span{ _ekey.get(), header.EKeySpec.HashSize }, std::endian::little);

        _especIndex = stream.Read<uint32_t>(std::endian::big);
        _fileSize = ReadUInt40(stream, std::endian::big);
    }

    /* static */ size_t Encoding::EKeySpecPageTable::HashSize(Header const& header) {
        return header.EKeySpec.HashSize;
    }

    // ^^^ EKeySpecPageTable / Encoding vvv

    Encoding::Encoding(io::IReadableStream& stream) {
        Header header{ stream };

        stream.SkipRead(header.ESpecBlockSize); // Skip ESpec strings

        size_t pageStart = stream.GetReadCursor() + header.CEKey.PageCount * (header.CEKey.HashSize + 0x10);

        _cekeyPageCount = header.CEKey.PageCount;
        _cekeyPages = std::make_unique<Page<CEKeyPageTable>[]>(header.CEKey.PageCount);

        for (size_t i = 0; i < header.CEKey.PageCount; ++i) {
            size_t pageOffset = pageStart + i * header.CEKey.PageSize;
            size_t pageEnd = pageOffset + header.CEKey.PageSize;

            new (&_cekeyPages[i]) Page<CEKeyPageTable>(stream, header, pageOffset, pageEnd);
        }
    }

    Encoding::Encoding(Encoding&& other) noexcept
        : _cekeyPageCount(other._cekeyPageCount), _cekeyPages(std::move(other._cekeyPages)), _keySpecPageTables(std::move(other._keySpecPageTables))
    {
        other._cekeyPageCount = 0;
    }

    Encoding& Encoding::operator = (Encoding&& other) noexcept {
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
}
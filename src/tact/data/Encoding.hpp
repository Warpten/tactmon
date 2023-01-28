#pragma once

#include <io/IStream.hpp>

#include <cstdint>
#include <memory>
#include <span>
#include <vector>

namespace tact::data {
    struct Header;

    struct Empty { };

    template <typename T, bool Indexed = false>
    struct Page {
        Page() = default;

        Page(io::IReadableStream& stream, Header const& header, size_t pageOffset, size_t pageEnd) {
            _hashSize = T::HashSize(header);

            if constexpr (!Indexed) {
                stream.SkipRead(_hashSize + 0x10);
            } else {
                _index = std::make_unique<uint8_t[]>(_hashSize + 0x10);
                stream.Read(std::span { _index.get(), _hashSize + 0x10 }, std::endian::little);
            }

            size_t position = stream.GetReadCursor();
            stream.SeekRead(pageOffset);

            while (stream.GetReadCursor() < pageEnd) {
                T pageEntry{ stream, header };
                // Stop if the entry read was mostly padding bytes, or if we read past the end of the page in the process (alignment bytes)
                if (!pageEntry || stream.GetReadCursor() > pageEnd)
                    break;

                _entries.emplace_back(std::move(pageEntry));
            }
            
            _entries.shrink_to_fit();
            stream.SeekRead(position);
        }

        T const& operator [] (size_t index) const { return _entries.at(index); }
        size_t size() const { return _entries.size(); }

    private:
        size_t _hashSize;
        std::vector<T> _entries;

        //! Because I can't be bothered specializing on the boolean.
        std::conditional_t<Indexed,
            std::unique_ptr<uint8_t[]>,
            Empty
        > _index;
    };

    struct Encoding final {
        explicit Encoding(io::IReadableStream& stream);
        Encoding(Encoding&& other) noexcept;

        ~Encoding();

        Encoding& operator = (Encoding&& other) noexcept;

        struct CEKeyPageTable final {
            CEKeyPageTable(io::IReadableStream& stream, Header const& header);
            CEKeyPageTable(CEKeyPageTable&& other) noexcept;

            CEKeyPageTable& operator = (CEKeyPageTable&& other) noexcept;

            operator bool() const;

            static size_t HashSize(Header const& header);

            size_t size() const;
            size_t fileSize() const;

            std::span<uint8_t const> operator [] (size_t index) const;

            std::span<uint8_t const> ckey() const;

        private:
            size_t _ekeySize;
            size_t _ckeySize;

            uint64_t _fileSize; // Of the non-encoded version of the file
            std::unique_ptr<uint8_t[]> _ckey;
            std::unique_ptr<uint8_t[]> _ekeys;
            uint8_t _keyCount;
        };

        struct EKeySpecPageTable final {
            EKeySpecPageTable(io::IReadableStream& stream, Header const& header);
            EKeySpecPageTable(EKeySpecPageTable&& other) noexcept;

            EKeySpecPageTable& operator = (EKeySpecPageTable&& other) noexcept;

            operator bool() const;

            static size_t HashSize(Header const& header);

        private:
            std::unique_ptr<uint8_t[]> _ekey = nullptr;
            uint32_t _especIndex = 0; // Not an offset but an index into the ESpec string block.
            uint64_t _fileSize = 0;   // Of the encoded version of the file.
        };

        size_t count() const;
        size_t specCount() const;

    private:
        size_t _cekeyPageCount = 0;
        std::unique_ptr<Page<CEKeyPageTable, false>[]> _cekeyPages;
        std::unique_ptr<Page<EKeySpecPageTable, false>[]> _keySpecPageTables;
    };
}

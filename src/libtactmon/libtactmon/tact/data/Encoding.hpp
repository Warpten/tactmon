#pragma once

#include "libtactmon/tact/CKey.hpp"
#include "libtactmon/tact/EKey.hpp"
#include "libtactmon/tact/data/FileLocation.hpp"

#include <cstdint>
#include <memory>
#include <optional>
#include <span>
#include <vector>

namespace libtactmon::io {
    struct IReadableStream;
}

namespace libtactmon::tact::data {
    struct Empty { };

    struct Encoding final {
        explicit Encoding(io::IReadableStream& stream);
        Encoding(Encoding&& other) noexcept;

        ~Encoding();

        Encoding& operator = (Encoding&& other) noexcept;

    public:
        [[nodiscard]] std::size_t GetContentKeySize() const;

        [[nodiscard]] std::size_t count() const;

        [[nodiscard]] std::optional<tact::data::FileLocation> FindFile(tact::CKey const& ckey) const;

    private:
        struct Header {
            uint16_t Signature = 0;
            uint8_t Version = 0;
            struct {
                uint32_t PageSize = 0;
                uint32_t PageCount = 0;
            } CEKey, EKeySpec;
            uint32_t EncodingKeySize = 0;
            uint32_t ContentKeySize = 0;
            uint32_t ESpecBlockSize = 0;

            explicit Header(io::IReadableStream& stream);
        };


        template <typename T, bool Indexed = false>
        struct Page {
            Page() = default;

            Page(io::IReadableStream& stream, Header const& header, std::size_t pageOffset, std::size_t pageEnd) {
                std::size_t hashSize = T::HashSize(header);

                if constexpr (!Indexed) {
                    stream.SkipRead(hashSize + 0x10);
                }
                else {
                    _index = std::make_unique<uint8_t[]>(hashSize + 0x10);
                    stream.Read(std::span{ _index.get(), hashSize + 0x10 }, std::endian::little);
                }

                std::size_t position = stream.GetReadCursor();
                stream.SeekRead(pageOffset);

                while (stream.GetReadCursor() < pageEnd) {
                    T pageEntry { stream, header };
                    // Stop if the entry read was mostly padding bytes, or if we read past the end of the page in the process (alignment bytes)
                    if (!pageEntry || stream.GetReadCursor() > pageEnd)
                        break;

                    _entries.emplace_back(std::move(pageEntry));
                }

                _entries.shrink_to_fit();
                stream.SeekRead(position);
            }

            T const& operator [] (size_t index) const { return _entries.at(index); }
            [[nodiscard]] std::size_t size() const { return _entries.size(); }

        private:
            std::vector<T> _entries;

            //! Because I can't be bothered specializing on the boolean.
            [[no_unique_address]] std::conditional_t<Indexed,
                std::unique_ptr<uint8_t[]>,
                Empty
            > _index;
        };


        struct CEKeyPageTable final {
            CEKeyPageTable(io::IReadableStream& stream, Header const& header);
            CEKeyPageTable(CEKeyPageTable&& other) noexcept;

            CEKeyPageTable& operator = (CEKeyPageTable&& other) noexcept;

            explicit operator bool() const;

            static std::size_t HashSize(Header const& header);

            [[nodiscard]] std::size_t keyCount() const { return _keyCount; }
            [[nodiscard]] std::size_t fileSize() const { return _fileSize; }

            [[nodiscard]] tact::EKey ekey(size_t index, Encoding const& owner) const;
            [[nodiscard]] tact::CKey ckey(Encoding const& owner) const;

        private:
            friend struct Encoding;

            uint64_t _fileSize; // Of the non-encoded version of the file
            std::vector<uint8_t> _ckey;
            std::vector<uint8_t> _ekeys;
            uint8_t _keyCount;
        };

        struct EKeySpecPageTable final {
            EKeySpecPageTable(io::IReadableStream& stream, Header const& header);
            EKeySpecPageTable(EKeySpecPageTable&& other) noexcept;

            EKeySpecPageTable& operator = (EKeySpecPageTable&& other) noexcept;

            explicit operator bool() const;

            static std::size_t HashSize(Header const& header);

        private:
            std::vector<uint8_t> _ekey;
            uint32_t _especIndex = 0; // Not an offset but an index into the ESpec string block.
            uint64_t _fileSize = 0;   // Of the encoded version of the file.
        };

        Header _header;

        std::vector<Page<CEKeyPageTable, false>> _cekeyPages;
        std::vector<Page<EKeySpecPageTable, false>> _keySpecPageTables;
    };
}

#pragma once

#include "libtactmon/tact/CKey.hpp"
#include "libtactmon/tact/data/product/Product.hpp"

#include <cstdint>
#include <optional>
#include <string_view>
#include <vector>

namespace libtactmon::io {
    struct IReadableStream;
}

namespace libtactmon::tact::data::product::wow {
    struct Root final {
        enum class ContentFlags : uint32_t {
            Install            = 0x00000004, // file is in install manifest
            LoadOnWindows      = 0x00000008, // macOS clients do not read block if flags & 0x108 != 0
            LoadOnMacOS        = 0x00000010, // windows clients do not read block if flags & 0x110 != 0
            x86_32             = 0x00000020, // install manifest file only - load on 32 bit systems
            x86_64             = 0x00000040, // install manifest file only - load on 64 bit systems
            LowViolence        = 0x00000080,
            DoNotLoad          = 0x00000100, // neither macOS nor windows clients read blocks with this flag set. LoadOnMysteryPlatform??
            UpdatePlugin       = 0x00000800, // only ever set for UpdatePlugin.dll and UpdatePlugin.dylib
            ARM64              = 0x00008000, // install manifest file only - load on ARM64 systems
            Encrypted          = 0x08000000,
            NoNameHash         = 0x10000000,
            UncommonResolution = 0x20000000, // denotes non-1280px wide cinematics
            Bundle             = 0x40000000,
            NoCompression      = 0x80000000,
        };

        enum class LocaleFlags : uint32_t {
            enUS = 0x00000002,
            koKR = 0x00000004,

            frFR = 0x00000010,
            deDE = 0x00000020,
            zhCN = 0x00000040,
            esES = 0x00000080,
            zhTW = 0x00000100,
            enGB = 0x00000200,
            enCN = 0x00000400,
            enTW = 0x00000800,
            esMX = 0x00001000,
            ruRU = 0x00002000,
            ptBR = 0x00004000,
            itIT = 0x00008000,
            ptPT = 0x00010000,
        };

        static std::optional<Root> Parse(io::IReadableStream& stream, size_t contentKeySize);

    private:
        Root();

    public:
        Root(Root&& other) noexcept;

        Root& operator = (Root&& other) noexcept;

        std::optional<tact::CKey> FindFile(uint32_t fileDataID) const;
        std::optional<tact::CKey> FindFile(std::string_view fileName) const;

        size_t size() const { return _entries.size(); }

        struct Entry {
            tact::CKey ContentKey;
            uint32_t FileDataID = 0;
            uint64_t NameHash = 0;

            Entry(Entry&& other) noexcept;
            Entry& operator = (Entry&& other) noexcept;

            Entry(Entry const& other) : ContentKey(other.ContentKey), FileDataID(other.FileDataID), NameHash(other.NameHash) { }

            Entry(tact::CKey contentKey, uint32_t fileDataID, uint64_t nameHash) : ContentKey(contentKey), FileDataID(fileDataID), NameHash(nameHash) { }
        };

        struct Block {
            ContentFlags Content;
            LocaleFlags Locales;

            std::vector<Entry> entries;
        };

    private:
        std::vector<Block> _blocks;
    };
}

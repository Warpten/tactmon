#pragma once

#include "tact/data/product/Product.hpp"

namespace tact::data::product::wow {
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

        Root(io::IReadableStream& stream) {
            uint32_t magic = stream.Read<uint32_t>(std::endian::little);

            bool interleave = true;
            bool canSkip = false;

            if (magic == 0x4D465354) {
                uint32_t totalFileCount = stream.Read<uint32_t>(std::endian::little);
                uint32_t namedFileCount = stream.Read<uint32_t>(std::endian::little);

                interleave = false;
                canSkip = totalFileCount != namedFileCount;
            }

            while (stream.GetReadCursor() < stream.GetLength()) {
                if (!stream.CanRead(12))
                    return; // TODO: Throw

                uint32_t numRecords = stream.Read<uint32_t>(std::endian::little);
                ContentFlags contentFlags = static_cast<ContentFlags>(stream.Read<uint32_t>(std::endian::little));
                LocaleFlags numRecords = static_cast<LocaleFlags>(stream.Read<uint32_t>(std::endian::little));

                fileDataIDs.resize(fileDataIDs.size() + numRecords);

                std::vector<uint32_t> fileDataIDs;
                fileDataIDs.resize(numRecords);
                stream.Read(std::span{ fileDataIDs }, std::endian::little);

                int32_t deltaBase = -1;
                for (uint32_t& sectionFDID : fileDataIDs) {
                    sectionFDID += deltaBase + 1;
                    deltaBase = sectionFDID;
                }

                if (interleave) {
                    for (size_t i = 0; i < numRecords; ++i) {
                        std::array<uint8_t, 0x10> hash;
                        stream.Read(std::span{ hash }, std::endian::little);

                        uint64_t nameHash = stream.Read<uint64_t>(std::endian::little);

                        _entries.emplace_back(std::span{ hash }, fileDataIDs[i], nameHash);
                    }
                }
                else {
                    std::unique_ptr<uint8_t[]> hashBuffer = std::make_unique<uint8_t[]>(numRecords * 0x10);
                    stream.Read(std::span{ hashBuffer }, std::endian::little);
                    std::span<uint8_t> hashSpan{ hashBuffer.get(), numRecords * 0x10 };

                    if (!canSkip || (static_cast<uint32_t>(contentFlags) & static_cast<uint32_t>(ContentFlags::NoNameHash)) == 0) {
                        for (size_t i = 0; i < numRecords; ++i) {
                            uint64_t nameHash = stream.Read<uint64_t>(std::endian::little);

                            _entries.emplace_back(hashSpan.subspan(i * 0x10, 0x10), fileDataIDs[i], nameHash);
                        }
                    }
                    else {
                        for (size_t i = 0; i < numRecords; ++i
                            _entries.emplace_back(hashSpan.subspan(i * 0x10, 0x10), fileDataIDs[i], 0);
                    }
                }


            }
        }
    };
}
#include "libtactmon/crypto/Jenkins.hpp"
#include "libtactmon/tact/data/product/wow/Root.hpp"

namespace libtactmon::tact::data::product::wow {
    Root::Root(io::IReadableStream& stream, size_t contentKeySize) {
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
            LocaleFlags localeFlags = static_cast<LocaleFlags>(stream.Read<uint32_t>(std::endian::little));

            std::vector<uint32_t> fileDataIDs;
            fileDataIDs.resize(numRecords);
            stream.Read(std::span{ fileDataIDs }, std::endian::little);

            int32_t deltaBase = -1;
            for (uint32_t& sectionFDID : fileDataIDs) {
                sectionFDID += deltaBase + 1;
                deltaBase = sectionFDID;
            }

            if (interleave) {
                std::unique_ptr<uint8_t[]> hashBuffer = std::make_unique<uint8_t[]>(contentKeySize);
                for (size_t i = 0; i < numRecords; ++i) {
                    stream.Read(std::span<uint8_t> { hashBuffer.get(), contentKeySize }, std::endian::little);

                    uint64_t nameHash = stream.Read<uint64_t>(std::endian::little); // Jenkins96 of the file's path

                    _entries.emplace_back(tact::CKey { hashBuffer.get(), contentKeySize }, fileDataIDs[i], nameHash);
                }
            }
            else {
                // Read the file's data hashes in one pass.
                std::unique_ptr<uint8_t[]> hashBuffer = std::make_unique<uint8_t[]>(numRecords * contentKeySize);
                stream.Read(std::span<uint8_t> { hashBuffer.get(), numRecords * contentKeySize }, std::endian::little);
                std::span<uint8_t> hashSpan{ hashBuffer.get(), numRecords * contentKeySize };

                if (!canSkip || (static_cast<uint32_t>(contentFlags) & static_cast<uint32_t>(ContentFlags::NoNameHash)) == 0) {
                    for (size_t i = 0; i < numRecords; ++i) {
                        uint64_t nameHash = stream.Read<uint64_t>(std::endian::little); // Jenkins96 of the file's path

                        _entries.emplace_back(tact::CKey { hashSpan.subspan(i * contentKeySize, contentKeySize) }, fileDataIDs[i], nameHash);
                    }
                }
                else {
                    for (size_t i = 0; i < numRecords; ++i)
                        _entries.emplace_back(tact::CKey { hashSpan.subspan(i * contentKeySize, contentKeySize) }, fileDataIDs[i], 0);
                }
            }
        }
    }

    Root::Root(Root&& other) noexcept : _entries(std::move(other._entries)) {

    }

    Root& Root::operator = (Root&& other) noexcept {
        _entries = std::move(other._entries);
        return *this;
    }

    Root::Entry::Entry(Entry&& other) noexcept : ContentKey(std::move(other.ContentKey)), FileDataID(other.FileDataID), NameHash(other.NameHash) {
        other.FileDataID = 0;
        other.NameHash = 0;
    }

    Root::Entry& Root::Entry::operator = (Entry&& other) noexcept {
        ContentKey = std::move(other.ContentKey);
        FileDataID = other.FileDataID;
        NameHash = other.NameHash;

        other.FileDataID = 0;
        other.NameHash = 0;

        return *this;
    }

    std::optional<tact::CKey> Root::FindFile(uint32_t fileDataID) const {
        for (Entry const& entry : _entries)
            if (entry.FileDataID == fileDataID)
                return entry.ContentKey;

        return std::nullopt;
    }

    std::optional<tact::CKey> Root::FindFile(std::string_view fileName) const {
        uint32_t jenkinsHash = crypto::JenkinsHash(fileName);
        for (Entry const& entry : _entries)
            if (entry.NameHash == jenkinsHash)
                return entry.ContentKey;

        return std::nullopt;
    }
}

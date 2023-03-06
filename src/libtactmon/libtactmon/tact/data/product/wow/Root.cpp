#include "libtactmon/crypto/Jenkins.hpp"
#include "libtactmon/tact/data/product/wow/Root.hpp"
#include "libtactmon/utility/Endian.hpp"

#include <boost/asio/io_context.hpp>
#include <boost/asio/thread_pool.hpp>
#include <boost/thread/future.hpp>

namespace libtactmon::tact::data::product::wow {
    struct PageInfo {
        uint32_t numRecords;
        ContentFlags contentFlags;
        LocaleFlags localeFlags;

        std::optional<std::span<const uint8_t>> data;
        size_t offset;
        size_t length;
    };

    std::optional<Root::Block> ParseBlock(PageInfo const& pageInfo) {
        Root::Block block;
        block.Content = pageInfo.contentFlags;
        block.Locales = pageInfo.localeFlags;

        block.entries.reserve(pageInfo.numRecords);

        // Get some stream here, around the span
        io::SpanStream stream { pageInfo.data }; // ...

        std::span<uint32_t const> fileDataIDs = stream.Data<uint32_t>().subspan(0, pageInfo.numRecords);
        stream.SkipRead(pageInfo.numRecords * sizeof(uint32_t));

        for (size_t i = 0; i < numRecords; ++i) {
            fileDataID += utility::to_endianness<std::endian::little>(fileDataIDs[i]) + 1;

            std::span<const uint8_t> hashSpan = stream.Data<uint8_t>().subspan(0, contentKeySize);
            stream.SkipRead(contentKeySize);

            uint64_t nameHash = stream.Read<uint64_t>(std::endian::little); // Jenkins96 of the file's path

            _entries.emplace_back(tact::CKey{ hashSpan }, fileDataID, nameHash);
        }
    }

    /* static */ std::optional<Root> Root::Parse(io::IReadableStream& stream, size_t contentKeySize) {
        if (!stream.CanRead(sizeof(uint32_t)))
            return std::nullopt;

        uint32_t magic = stream.Read<uint32_t>(std::endian::little);

        bool interleave = true;
        bool canSkip = false;

        if (magic == 0x4D465354) {
            if (!stream.CanRead(sizeof(uint32_t) * 2))
                return std::nullopt;

            uint32_t totalFileCount = stream.Read<uint32_t>(std::endian::little);
            uint32_t namedFileCount = stream.Read<uint32_t>(std::endian::little);

            interleave = false;
            canSkip = totalFileCount != namedFileCount;
        }

        // Generate segments of data to parse in parallel
        boost::asio::thread_pool pool { 4 };

        std::list<std::future<std::optional<Block>>> futures;

        while (stream.GetReadCursor() < stream.GetLength()) {
            if (!stream.CanRead(12))
                return std::nullopt;

            PageInfo page;
            page.numRecords = stream.Read<uint32_t>(std::endian::little);
            page.contentFlags = static_cast<ContentFlags>(stream.Read<uint32_t>(std::endian::little));
            page.localeFlags = static_cast<LocaleFlags>(stream.Read<uint32_t>(std::endian::little));

            page.offset = stream.GetReadCursor();

            size_t length = sizeof(uint32_t) * page.numRecords; // u32 fileDataID[numRecords]
            if (interleave) {
                length += contentKeySize * page.numRecords; // u8 contentKeys[contentKeySize][numRecords]
                length += sizeof(uint64_t) * page.numRecords; // u64 nameHash[numRecords]
            }
            else {
                if (!canSkip || (static_cast<uint32_t>(contentFlags) & static_cast<uint32_t>(ContentFlags::NoNameHash)) == 0) {
                    length += sizeof(uint64_t) * page.numRecords; // u64 nameHash[numRecords]
                }
            }

            if (!stream.CanRead(page.length))
                return std::nullopt;

            page.data.emplace(stream.Data<uint8_t>().subspan(0, length));
            stream.SkipRead(page.length);

            auto blockLoader = std::make_shared<boost::packaged_task<std::optional<Block>>>([pageInfo = std::move(page)]() {
                return ParseBlock(pageInfo);
            });

            futures.push_back(blockLoader.get_future());
            boost::asio::post(pool, [blockLoader]() { (*blockLoader)(); });
        }

        Root instance;

        instance._blocks.reserve(futures.size());
        for (auto&& future : boost::when_all(futures.begin(), futures.end()).get()) {
            std::optional<Block> value = future.get();
            if (!value.has_value())
                return std::nullopt;

            instance._blocks.emplace_back(std::move(*value));
        }

        pool.join();

        return instance;
    }

    /*Root::Root(io::IReadableStream& stream, size_t contentKeySize) {
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

            if (!stream.CanRead(numRecords * sizeof(uint32_t))) {
                _entries.clear();
                return;
            }

            std::span<uint32_t const> fileDataIDs = stream.Data<uint32_t>().subspan(0, numRecords);
            stream.SkipRead(numRecords * sizeof(uint32_t));

            _entries.reserve(_entries.size() + numRecords);

            if (interleave) {
                if (!stream.CanRead((contentKeySize + sizeof(uint64_t)) * numRecords)) {
                    _entries.clear();
                    return;
                }

                uint32_t fileDataID = -1;
                for (size_t i = 0; i < numRecords; ++i) {
                    fileDataID += utility::to_endianness<std::endian::little>(fileDataIDs[i]) + 1;

                    std::span<const uint8_t> hashSpan = stream.Data<uint8_t>().subspan(0, contentKeySize);
                    stream.SkipRead(contentKeySize);

                    uint64_t nameHash = stream.Read<uint64_t>(std::endian::little); // Jenkins96 of the file's path

                    _entries.emplace_back(tact::CKey{ hashSpan }, fileDataID, nameHash);
                }
            }
            else {
                // Read the file's data hashes in one pass.
                if (!stream.CanRead(numRecords * contentKeySize)) {
                    _entries.clear();
                    return;
                }

                std::span<const uint8_t> hashSpan = stream.Data<uint8_t>().subspan(0, numRecords * contentKeySize);
                stream.SkipRead(hashSpan.size());

                uint32_t fileDataID = -1;

                if (!canSkip || (static_cast<uint32_t>(contentFlags) & static_cast<uint32_t>(ContentFlags::NoNameHash)) == 0) {
                    if (!stream.CanRead(numRecords * sizeof(uint64_t))) {
                        _entries.clear();
                        return;
                    }

                    for (size_t i = 0; i < numRecords; ++i) {
                        fileDataID += utility::to_endianness<std::endian::little>(fileDataIDs[i]) + 1;

                        uint64_t nameHash = stream.Read<uint64_t>(std::endian::little); // Jenkins96 of the file's path

                        _entries.emplace_back(tact::CKey{ hashSpan.subspan(i * contentKeySize, contentKeySize) }, fileDataID, nameHash);
                    }
                }
                else {
                    for (size_t i = 0; i < numRecords; ++i) {
                        fileDataID += utility::to_endianness<std::endian::little>(fileDataIDs[i]) + 1;
                        _entries.emplace_back(tact::CKey{ hashSpan.subspan(i * contentKeySize, contentKeySize) }, fileDataID, 0);
                    }
                }
            }
        }
    }

    */
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
        // Can a bonary search be used on the blocks as well? This assumes blocks don't overlap and max(prev_block.fdids) < min(curr_block.fdids)
        for (Block const& block : _blocks) {
            // Can use binary search because we know fdids are ordered.
            auto itr = std::lower_bound(block.entries.begin(), block.entries.end(), fileDataID, [](Entry const& entry, uint32_t fdid) { return entry.FileDataID < fdid; });
            if (itr != block.entries.end())
                return itr->ContentKey;
        }

        return std::nullopt;
    }

    std::optional<tact::CKey> Root::FindFile(std::string_view fileName) const {
        uint32_t jenkinsHash = crypto::JenkinsHash(fileName);

        // Unfortunately, no binary search here.
        for (Entry const& entry : _entries)
            if (entry.NameHash == jenkinsHash)
                return entry.ContentKey;

        return std::nullopt;
    }
}

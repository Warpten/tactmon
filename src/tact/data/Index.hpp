#pragma once

#include "tact/EKey.hpp"

#include <boost/container_hash/hash.hpp>

#include <cstdint>
#include <string_view>
#include <unordered_set>
#include <vector>

namespace io {
    struct IReadableStream;
}

namespace tact::data {
    struct Index final {
        explicit Index(std::string_view hash, io::IReadableStream& stream);

        std::string_view name() const { return _archiveName; }

        struct Entry {
            friend struct Index;

            Entry(io::IReadableStream& stream, size_t sizeBytes, size_t offsetBytes, size_t keyOffset);

        public:
            uint64_t size() const { return _size; }
            uint64_t offset() const { return _offset; }

            std::span<const uint8_t> key(Index const& index) const;

        private:
            size_t _keyOffset = 0;
            uint64_t _size = 0;
            uint64_t _offset = 0;
        };

        const Entry* operator [] (tact::EKey const& key) const;

    private:
        size_t _keySizeBytes;
        std::vector<uint8_t> _keyBuffer;
        std::vector<Entry> _entries;
        std::string _archiveName;
    };
}

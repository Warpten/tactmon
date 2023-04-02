#pragma once

#include "libtactmon/tact/EKey.hpp"

#include <cstdint>
#include <string_view>
#include <unordered_set>
#include <vector>

namespace libtactmon::io {
    struct IReadableStream;
}

namespace libtactmon::tact::data {
    struct Index final {
        explicit Index(std::string_view hash, io::IReadableStream& stream);

        std::string_view name() const { return _archiveName; }

        struct Entry {
            friend struct Index;

            Entry(io::IReadableStream& stream, size_t sizeBytes, size_t offsetBytes, size_t keyOffset);

        public:
            size_t size() const { return _size; }
            size_t offset() const { return _offset; }

            std::span<const uint8_t> key(Index const& index) const;

        private:
            size_t _keyOffset = 0;
            size_t _size = 0;
            size_t _offset = 0;
        };

        const Entry* operator [] (tact::EKey const& key) const;

    private:
        size_t _keySizeBytes;
        std::vector<uint8_t> _keyBuffer;
        std::vector<Entry> _entries;
        std::string _archiveName;
    };
}

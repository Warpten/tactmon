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

        [[nodiscard]] std::string_view name() const { return _archiveName; }

        struct Entry {
            friend struct Index;

            Entry(io::IReadableStream& stream, std::size_t sizeBytes, std::size_t offsetBytes, std::size_t keyOffset);

        public:
            [[nodiscard]] std::size_t size() const { return _size; }
            [[nodiscard]] std::size_t offset() const { return _offset; }

            [[nodiscard]] std::span<const uint8_t> key(Index const& index) const;

        private:
            std::size_t _keyOffset = 0;
            std::size_t _size = 0;
            std::size_t _offset = 0;
        };

        const Entry* operator [] (tact::EKey const& key) const;

    private:
        std::size_t _keySizeBytes;
        std::vector<uint8_t> _keyBuffer;
        std::vector<Entry> _entries;
        std::string _archiveName;
    };
}

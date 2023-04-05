#pragma once

#include "libtactmon/tact/CKey.hpp"

#include <cstdint>
#include <list>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace libtactmon::io {
    struct IReadableStream;
}

namespace libtactmon::tact::data {
    struct Install final {
        static std::optional<Install> Parse(io::IReadableStream& stream);

        struct Tag final {
            friend struct Install;

            Tag(io::IReadableStream& stream, std::size_t bitmaskSize, std::string_view name);

            Tag(Tag&& other) noexcept;
            Tag& operator = (Tag&& other) noexcept;

            Tag(Tag const&) = delete;
            Tag& operator = (Tag const&) = delete;

        public:
            [[nodiscard]] std::string_view name() const { return _name; }
            [[nodiscard]] uint8_t type() const { return _type; }

            [[nodiscard]] bool Matches(std::size_t fileIndex) const;

        private:
            std::string_view _name;
            uint8_t _type;

            std::size_t _bitmaskSize;
            std::unique_ptr<uint8_t[]> _bitmask;
        };

        [[nodiscard]] std::size_t size() const { return _entries.size(); }

        [[nodiscard]] std::optional<tact::CKey> FindFile(std::string_view fileName) const;

    private:
        Install();

        struct Entry {
            friend struct Install;

            Entry(io::IReadableStream& stream, size_t hashSize, std::string const& name);

            [[nodiscard]] std::string_view name() const { return _name; }

            [[nodiscard]] tact::CKey const& ckey() const { return _hash; }

        private:
            std::string _name;
            std::size_t _fileSize;
            
            tact::CKey _hash;

            std::vector<Tag*> _tags;
        };

        std::list<std::string> _tagNames; // Actual storage for tag names
        std::unordered_map<std::string_view, Tag> _tags;

        std::vector<Entry> _entries;
    };
}

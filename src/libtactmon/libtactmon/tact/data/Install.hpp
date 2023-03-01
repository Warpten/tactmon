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

            Tag(Install const& install, io::IReadableStream& stream, size_t bitmaskSize, std::string_view name);

            Tag(Tag&& other) noexcept;
            Tag& operator = (Tag&& other) noexcept;

            Tag(Tag const&) = delete;
            Tag& operator = (Tag const&) = delete;

        public:
            std::string_view name() const { return _name; }
            uint8_t type() const { return _type; }

            bool Matches(size_t fileIndex) const;

        private:
            Install const& _install;
            std::string_view _name;
            uint8_t _type;

            size_t _bitmaskSize;
            std::unique_ptr<uint8_t[]> _bitmask;
        };

        size_t size() const { return _entries.size(); }

        std::optional<tact::CKey> FindFile(std::string_view fileName) const;

    private:
        Install();

        struct Entry {
            friend struct Install;

            Entry(Install const& install, io::IReadableStream& stream, size_t hashSize, std::string const& name);

            std::string_view name() const { return _name; }

            tact::CKey const& ckey() const { return _hash; }

        private:
            Install const& _install;

            std::string _name;
            size_t _fileSize;
            
            tact::CKey _hash;

            std::vector<Tag*> _tags;
        };

        std::list<std::string> _tagNames; // Actual storage for tag names
        std::unordered_map<std::string_view, Tag> _tags;

        std::vector<Entry> _entries;
    };
}

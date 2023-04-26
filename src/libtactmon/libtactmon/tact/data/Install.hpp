#pragma once

#include "libtactmon/detail/Export.hpp"
#include "libtactmon/tact/CKey.hpp"
#include "libtactmon/Result.hpp"

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
    struct LIBTACTMON_API Install final {
        static Result<Install> Parse(io::IReadableStream& stream);

        struct LIBTACTMON_API Tag final {
            friend struct Install;

            Tag(io::IReadableStream& stream, std::size_t bitmaskSize, std::string&& name);

            Tag(Tag&& other) noexcept;
            Tag& operator = (Tag&& other) noexcept;

            Tag(Tag const&);
            Tag& operator = (Tag const&);

        public:
            [[nodiscard]] std::string_view name() const { return _name; }
            [[nodiscard]] uint8_t type() const { return _type; }

            [[nodiscard]] bool Matches(std::size_t fileIndex) const;

        private:
            std::string _name;
            uint8_t _type;

            std::size_t _bitmaskSize;
            std::unique_ptr<uint8_t[]> _bitmask;
        };

        /**
         * Returns the number of file entries in this manifest.
         */
        [[nodiscard]] std::size_t size() const { return _entries.size(); }

        /**
         * Returns the content key of a file in this install manifest, or an empty manifest if no information could be found for said file.
         *
         * @param[in] fileName The complete path to the file for which the caller expects a content key.
         */
        [[nodiscard]] std::optional<tact::CKey> FindFile(std::string_view fileName) const;

    private:
        Install();

        struct Entry {
            friend struct Install;

            Entry(io::IReadableStream& stream, std::size_t hashSize, std::string const& name);

            [[nodiscard]] std::string_view name() const { return _name; }

            [[nodiscard]] tact::CKey const& ckey() const { return _hash; }

        private:
            std::string _name;
            std::size_t _fileSize;
            
            tact::CKey _hash;

            std::vector<Tag*> _tags;
        };

        std::vector<Tag> _tags;

        std::vector<Entry> _entries;
    };
}

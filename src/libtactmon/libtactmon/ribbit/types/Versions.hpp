#pragma once

#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace libtactmon::ribbit::types {
    namespace versions {
        struct Record {
            std::string Region;
            std::string BuildConfig;
            std::string CDNConfig;
            std::string KeyRing;
            uint32_t BuildID = 0;
            std::string VersionsName;
            std::string ProductConfig;

            static std::optional<Record> Parse(std::string_view input);
        };
    }

    struct Versions {
        std::vector<versions::Record> Records;
        std::size_t SequenceID = 0;
    };
}

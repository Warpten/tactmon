#pragma once

#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace libtactmon::ribbit::types {
    namespace cdns {
        struct Record {
            std::string Name;
            std::string Path;
            std::vector<std::string> Hosts;
            std::vector<std::string> Servers;
            std::string ConfigPath;

            static std::optional<Record> Parse(std::string_view input);
        };
    }

    using CDNs = std::vector<cdns::Record>;
}

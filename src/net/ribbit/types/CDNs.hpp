#pragma once

#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace net::ribbit::types {
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

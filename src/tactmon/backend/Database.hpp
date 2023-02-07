#pragma once

#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include <pqxx/connection>

namespace backend {
    struct Database final {
        Database(std::string_view username, std::string_view password, std::string_view host, uint64_t port, std::string_view name);

        struct Build {
            uint32_t ID;
            std::string Name;
            std::string BuildConfig;
            std::string CdnConfig;

            static std::optional<Build> TryParse(pqxx::row const& row);
        };

        std::optional<Build> SelectBuild(std::string const& buildName);

        std::vector<Build> SelectBuilds(std::string const& productName);

    private:
        pqxx::connection _connection;
    };
}

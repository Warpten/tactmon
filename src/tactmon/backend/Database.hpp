#pragma once

#include "backend/Entities.hpp"
#include "backend/Queries.hpp"

#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include <pqxx/connection>

namespace backend {
    namespace entities = db::entities;

    struct Database final {
        Database(std::string_view username, std::string_view password, std::string_view host, uint64_t port, std::string_view name);

        std::optional<entities::build::Entity> SelectBuild(std::string const& buildName);

        std::vector<entities::build::dto::BuildName> SelectBuilds(std::string const& productName);

    private:
        pqxx::connection _connection;
    };
}

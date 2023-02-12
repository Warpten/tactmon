#pragma once

#include "backend/db/entity/Build.hpp"
#include "backend/db/repository/Build.hpp"

#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include <pqxx/connection>

#include <boost/asio/io_context.hpp>

namespace backend {
    struct Database final {
        Database(std::shared_ptr<boost::asio::io_context> context, 
            std::string_view username, std::string_view password, std::string_view host, uint64_t port, std::string_view name);

        db::repository::Build const& GetBuildRepository() const { return _buildRepository; }

    private:
        std::shared_ptr<pqxx::connection> _connection;

        db::repository::Build _buildRepository;
    };
}

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
        Database(boost::asio::io_context::strand strand,
            std::string_view username, std::string_view password, std::string_view host, uint64_t port, std::string_view name);
        
    private:
        pqxx::connection _connection;

    public:
        db::repository::Build builds;
    };
}

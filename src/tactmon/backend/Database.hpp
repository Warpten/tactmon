#pragma once

#include "backend/db/entity/Build.hpp"
#include "backend/db/repository/BoundChannel.hpp"
#include "backend/db/repository/Build.hpp"
#include "backend/db/repository/CommandState.hpp"
#include "backend/db/repository/Product.hpp"
#include "backend/db/repository/TrackedFile.hpp"
#include "utility/ThreadPool.hpp"

#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include <boost/asio/io_context.hpp>

#include <pqxx/connection>

#include <spdlog/async_logger.h>

namespace backend {
    struct Database final {
        Database(size_t threadCount, spdlog::async_logger& logger,
            std::string_view username, std::string_view password, std::string_view host, uint64_t port, std::string_view name);
        
    private:
        pqxx::connection _connection;
        utility::ThreadPool _threadPool;

    public:
        db::repository::Build builds;
        db::repository::Product products;
        db::repository::BoundChannel boundChannels;
        db::repository::TrackedFile trackedFiles;
        db::repository::CommandState commandStates;
    };
}

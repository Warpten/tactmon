#pragma once

#include "backend/ConnectionPool.hpp"
#include "utility/ThreadPool.hpp"

#include <cstdint>
#include <memory>
#include <string_view>

#include <spdlog/async_logger.h>

namespace backend {
    namespace db::repository {
        struct Build;
        struct Product;
        struct BoundChannel;
        struct TrackedFile;
        struct CommandState;
    }

    struct Database final {
        Database(std::size_t threadCount, spdlog::async_logger& logger,
            std::string_view username, std::string_view password, std::string_view host, uint64_t port, std::string_view name);

        ~Database();

    private:
        Pool _connectionPool;
        utility::ThreadPool _threadPool;

    public:
        std::unique_ptr<db::repository::Build> builds;
        std::unique_ptr<db::repository::Product> products;
        std::unique_ptr<db::repository::BoundChannel> boundChannels;
        std::unique_ptr<db::repository::TrackedFile> trackedFiles;
        std::unique_ptr<db::repository::CommandState> commandStates;
    };
}

#include "backend/Database.hpp"
#include "backend/db/repository/BoundChannel.hpp"
#include "backend/db/repository/Build.hpp"
#include "backend/db/repository/CommandState.hpp"
#include "backend/db/repository/Product.hpp"
#include "backend/db/repository/TrackedFile.hpp"

#include "utility/Literal.hpp"

#include <pqxx/pqxx>

namespace backend {
    Database::Database(std::size_t threadCount, spdlog::async_logger& logger,
        std::string_view username, std::string_view password, std::string_view host, uint64_t port, std::string_view name)
        : _connectionPool(username, password, host, port, name)
        , _threadPool(threadCount)
        , builds(std::make_unique<db::repository::Build>(_threadPool, _connectionPool, logger))
        , products(std::make_unique<db::repository::Product>(_threadPool, _connectionPool, logger))
        , boundChannels(std::make_unique<db::repository::BoundChannel>(_threadPool, _connectionPool, logger))
        , trackedFiles(std::make_unique<db::repository::TrackedFile>(_threadPool, _connectionPool, logger))
        , commandStates(std::make_unique<db::repository::CommandState>(_connectionPool, logger))
    {
    }

    Database::~Database() = default;
}

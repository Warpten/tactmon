#include "backend/Database.hpp"

#include "utility/Literal.hpp"

#include <fmt/format.h>
#include <optional>
#include <sstream>
#include <tuple>
#include <type_traits>
#include <vector>

#include <pqxx/pqxx>

namespace backend {
    Database::Database(size_t threadCount, spdlog::async_logger& logger,
        std::string_view username, std::string_view password, std::string_view host, uint64_t port, std::string_view name)
        : _connectionPool(username, password, host, port, name)
        , _threadPool(threadCount)
        , builds(_threadPool, _connectionPool, logger)
        , products(_threadPool, _connectionPool, logger)
        , boundChannels(_threadPool, _connectionPool, logger)
        , trackedFiles(_threadPool, _connectionPool, logger)
        , commandStates(_connectionPool, logger)
    {
    }
}

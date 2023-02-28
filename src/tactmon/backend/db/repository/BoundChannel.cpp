#include "backend/db/repository/BoundChannel.hpp"

#include <chrono>

namespace backend::db::repository {
    BoundChannel::BoundChannel(utility::ThreadPool& threadPool, pqxx::connection& connection, spdlog::async_logger& logger)
        : Base(threadPool, connection, logger)
    {
        entity::bound_channel::queries::Insert::Prepare(_connection, _logger);
        entity::bound_channel::queries::Delete::Prepare(_connection, _logger);
    }

    void BoundChannel::Insert(uint64_t guildID, uint64_t channelID, std::string const& productName) {
        Execute<entity::bound_channel::queries::Insert>(guildID, channelID, productName);
    }

    void BoundChannel::Delete(uint64_t channelID, std::string const& productName) {
        Execute<entity::bound_channel::queries::Delete>(channelID, productName);
    }
}

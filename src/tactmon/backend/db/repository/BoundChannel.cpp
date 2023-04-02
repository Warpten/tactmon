#include "backend/db/repository/BoundChannel.hpp"

#include <chrono>

namespace backend::db::repository {
    BoundChannel::BoundChannel(utility::ThreadPool& threadPool, pqxx::connection& connection, spdlog::async_logger& logger)
        : Base(threadPool, connection, logger)
    {
        entity::bound_channel::queries::Insert::Prepare(connection, logger);
        entity::bound_channel::queries::Delete::Prepare(connection, logger);
    }

    void BoundChannel::Insert(uint64_t guildID, uint64_t channelID, std::string productName) {
        entity::bound_channel::queries::Insert::Execute(_connection, guildID, channelID, std::move(productName));
    }

    void BoundChannel::Delete(uint64_t channelID, std::string productName) {
        entity::bound_channel::queries::Delete::Execute(_connection, channelID, std::move(productName));
    }
}

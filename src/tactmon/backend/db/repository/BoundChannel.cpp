#include "backend/db/repository/BoundChannel.hpp"

#include <chrono>

namespace backend::db::repository {
    BoundChannel::BoundChannel(utility::ThreadPool& threadPool, Pool& pool, spdlog::async_logger& logger)
        : Base(threadPool, pool, logger)
    {
        entity::bound_channel::queries::Insert::Prepare(pool, logger);
        entity::bound_channel::queries::Delete::Prepare(pool, logger);
    }

    void BoundChannel::Insert(uint64_t guildID, uint64_t channelID, std::string productName) {
        entity::bound_channel::queries::Insert::Execute(_pool, guildID, channelID, std::move(productName));
    }

    void BoundChannel::Delete(uint64_t channelID, std::string productName) {
        entity::bound_channel::queries::Delete::Execute(_pool, channelID, std::move(productName));
    }
}

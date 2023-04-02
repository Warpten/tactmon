#pragma once

#include "backend/db/repository/Repository.hpp"
#include "backend/db/entity/BoundChannel.hpp"
#include "utility/ThreadPool.hpp"

#include <optional>
#include <string>
#include <vector>

#include <pqxx/connection>

#include <spdlog/async_logger.h>

namespace backend::db::repository {
    struct BoundChannel : Repository<entity::bound_channel::Entity, entity::bound_channel::queries::Select, entity::bound_channel::id, true> {
        using Base = Repository<entity::bound_channel::Entity, entity::bound_channel::queries::Select, entity::bound_channel::id, true>;

        BoundChannel(utility::ThreadPool& threadPool, pqxx::connection& connection, spdlog::async_logger& logger);

        /**
         * Registers a bound channel.
         * 
         * @param[in] guildID     The ID of the Discord guild to which we are binding product push announcements to.
         * @param[in] channelID   The ID of the channel to which we are binding product push announcements to.
         * @param[in] productName The product we are unbinding from a channel.
         */
        void Insert(uint64_t guildID, uint64_t channelID, std::string productName);

        /**
         * Unbinds from a channel.
         * 
         * @param[in] channelID   The ID of the channel to which we are unbinding product push announcements to.
         * @param[in] productName The product we are unbinding from a channel.
         */
        void Delete(uint64_t channelID, std::string productName);
    };
}

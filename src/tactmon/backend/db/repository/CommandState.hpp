#pragma once

#include "backend/ConnectionPool.hpp"
#include "backend/db/repository/Repository.hpp"
#include "backend/db/entity/CommandState.hpp"
#include "utility/ThreadPool.hpp"

#include <optional>
#include <string>
#include <vector>

#include <pqxx/connection>

#include <spdlog/async_logger.h>

namespace backend::db::repository {
    struct CommandState : Repository<entity::command_state::Entity, entity::command_state::queries::Select, entity::command_state::id, false> {
        using Base = Repository<entity::command_state::Entity, entity::command_state::queries::Select, entity::command_state::id, false>;

        CommandState(Pool& pool, spdlog::async_logger& logger);

        std::optional<entity::command_state::Entity> FindCommand(std::string name);

        void InsertOrUpdate(std::string commandName, uint64_t hash, uint32_t version);
    };
}

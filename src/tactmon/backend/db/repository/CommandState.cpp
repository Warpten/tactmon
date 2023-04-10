#include "backend/db/repository/CommandState.hpp"

#include <chrono>

namespace backend::db::repository {
    CommandState::CommandState(Pool& pool, spdlog::async_logger& logger)
        : Base(pool, logger)
    {
        entity::command_state::queries::SelectByName::Prepare(pool, logger);
        entity::command_state::queries::InsertOrUpdate::Prepare(pool, logger);
    }

    std::optional<entity::command_state::Entity> CommandState::FindCommand(std::string name) {
        return entity::command_state::queries::SelectByName::ExecuteOne(_pool, std::move(name));
    }

    void CommandState::InsertOrUpdate(std::string commandName, uint64_t hash, uint32_t version) {
        entity::command_state::queries::InsertOrUpdate::ExecuteOne(_pool, std::move(commandName), hash, version);
    }
}

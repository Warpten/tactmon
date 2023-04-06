#include "backend/db/repository/CommandState.hpp"

#include <chrono>

namespace backend::db::repository {
    CommandState::CommandState(pqxx::connection& connection, spdlog::async_logger& logger)
        : Base(connection, logger)
    {
        entity::command_state::queries::SelectByName::Prepare(_connection, _logger);
        entity::command_state::queries::InsertOrUpdate::Prepare(_connection, _logger);
    }

    std::optional<entity::command_state::Entity> CommandState::FindCommand(std::string name) {
        return entity::command_state::queries::SelectByName::ExecuteOne(_connection, std::move(name));
    }

    void CommandState::InsertOrUpdate(std::string commandName, uint64_t hash, uint32_t version) {
        entity::command_state::queries::InsertOrUpdate::ExecuteOne(_connection, std::move(commandName), hash, version);
    }
}

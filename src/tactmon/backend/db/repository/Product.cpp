#include "backend/db/repository/Product.hpp"

#include <chrono>

namespace backend::db::repository {
    Product::Product(utility::ThreadPool& threadPool, pqxx::connection& connection, spdlog::async_logger& logger)
        : Base(threadPool, connection, logger)
    {
        entity::product::queries::SelById::Prepare(_connection, _logger);
        entity::product::queries::SelByName::Prepare(_connection, _logger);

        entity::product::queries::Insert::Prepare(_connection, _logger);
        entity::product::queries::Update::Prepare(_connection, _logger);
    }

    std::optional<entity::product::Entity> Product::GetByName(std::string const& buildName) const {
        return ExecuteOne<entity::product::queries::SelByName>(buildName);
    }

    void Product::Insert(std::string const& productName, uint64_t sequenceID) {
        ExecuteOne<entity::product::queries::Insert>(productName, sequenceID);
    }

    void Product::Update(std::string const& productName, uint64_t sequenceID) {
        ExecuteOne<entity::product::queries::Update>(productName, sequenceID);
    }
}

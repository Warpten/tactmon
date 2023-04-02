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

    std::optional<entity::product::Entity> Product::GetByName(std::string buildName) const {
        entity::product::queries::SelByName::ExecuteOne(_connection, std::move(buildName));
        return { };
    }

    void Product::Insert(std::string productName, uint64_t sequenceID) {
        entity::product::queries::Insert::ExecuteOne(_connection, std::move(productName), sequenceID);
    }

    void Product::Update(std::string productName, uint64_t sequenceID) {
        entity::product::queries::Update::ExecuteOne(_connection, sequenceID, std::move(productName));
    }
}

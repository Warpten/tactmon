#include "backend/db/repository/Product.hpp"

#include <chrono>

namespace backend::db::repository {
    Product::Product(utility::ThreadPool& threadPool, Pool& pool, spdlog::async_logger& logger)
        : Base(threadPool, pool, logger)
    {
        entity::product::queries::SelById::Prepare(pool, logger);
        entity::product::queries::SelByName::Prepare(pool, logger);

        entity::product::queries::Insert::Prepare(pool, logger);
        entity::product::queries::Update::Prepare(pool, logger);
    }

    std::optional<entity::product::Entity> Product::GetByName(std::string buildName) const {
        return entity::product::queries::SelByName::ExecuteOne(_pool, std::move(buildName));
    }

    void Product::Insert(std::string productName, uint64_t sequenceID) {
        entity::product::queries::Insert::ExecuteOne(_pool, std::move(productName), sequenceID);
    }

    void Product::Update(std::string productName, uint64_t sequenceID) {
        entity::product::queries::Update::ExecuteOne(_pool, sequenceID, std::move(productName));
    }
}

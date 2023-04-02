#pragma once

#include "backend/db/repository/Repository.hpp"
#include "backend/db/entity/Product.hpp"
#include "utility/ThreadPool.hpp"

#include <optional>
#include <string>
#include <vector>

#include <pqxx/connection>

#include <spdlog/async_logger.h>

namespace backend::db::repository {
    struct Product : Repository<entity::product::Entity, entity::product::queries::Select, entity::product::id, true> {
        using Base = Repository<entity::product::Entity, entity::product::queries::Select, entity::product::id, true>;

        Product(utility::ThreadPool& threadPool, pqxx::connection& connection, spdlog::async_logger& logger);

        /**
        * Returns the record for a build with the given name.
        * 
        * @param[in] buildName The name of the build to retrieve information for.
        */
        std::optional<entity::product::Entity> GetByName(std::string productName) const;

        /**
         * Registers a new product.
         * 
         * @param[in] productName The name of the product.
         * @param[in] sequenceID  The product's current sequence ID.
         */
        void Insert(std::string productName, uint64_t sequenceID);

        /**
         * Updates a product's sequence ID.
         * 
         * @param[in] productName The name of the product.
         * @param[in] sequenceID  The product's new sequence ID.
         */
        void Update(std::string productName, uint64_t sequenceID);
    };
}

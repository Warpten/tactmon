#pragma once

#include "backend/db/repository/Repository.hpp"
#include "backend/db/entity/Build.hpp"
#include "utility/ThreadPool.hpp"

#include <optional>
#include <string>
#include <vector>

#include <pqxx/connection>

#include <spdlog/async_logger.h>

namespace backend::db::repository {
    struct Build : Repository<entity::build::Entity, entity::build::queries::Select, entity::build::id, true> {
        using Base = Repository<entity::build::Entity, entity::build::queries::Select, entity::build::id, true>;

        Build(utility::ThreadPool& threadPool, pqxx::connection& connection, spdlog::async_logger& logger);
        
        /**
         * Returns the record for a build with the given name.
         * 
         * @param[in] buildName The name of the build to retrieve information for.
         * @param[in] region    The name of the region.
         */
        std::optional<entity::build::Entity> GetByBuildName(std::string buildName, std::string region) const;

        /**
         * Returns the name and the ID of all builds matching a specific product name.
         * 
         * @param[in] productName The name of the product.
         */
        std::vector<entity::build::dto::BuildName> GetByProductName(std::string productName) const;

        /**
         * Returns various informations about a specific product.
         * 
         * @param[in] productName The name of the product.
         */
        std::optional<entity::build::dto::BuildStatistics> GetStatisticsForProduct(std::string productName) const;

        std::optional<entity::build::Entity> Insert(std::string region, std::string productName, std::string buildName,
            std::string buildConfig, std::string cdnConfig);
    };
}

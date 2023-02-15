#pragma once

#include "backend/db/repository/Repository.hpp"
#include "backend/db/entity/Build.hpp"

#include <optional>
#include <string>
#include <vector>

#include <pqxx/connection>

namespace backend::db::repository {
    struct Build : Repository<entity::build::Entity, entity::build::queries::Select, entity::build::id, true> {
        using Base = Repository<entity::build::Entity, entity::build::queries::Select, entity::build::id, true>;

        Build(boost::asio::io_context::strand strand, pqxx::connection& connection);
        
        /**
         * Returns the record for a build with the given name.
         * 
         * @param[in] buildName The name of the build to retrieve information for.
         */
        std::optional<entity::build::Entity> GetByBuildName(std::string const& buildName) const;

        /**
         * Returns the name and the ID of all builds matching a specific product name.
         * 
         * @param[in] productName The name of the product.
         */
        std::vector<entity::build::dto::BuildName> GetByProductName(std::string const& productName) const;

        /**
         * Returns various informations about a specific product.
         * 
         * @param[in] productName The name of the product.
         */
        std::optional<entity::build::dto::BuildStatistics> GetStatisticsForProduct(std::string const& productName) const;
    };
}

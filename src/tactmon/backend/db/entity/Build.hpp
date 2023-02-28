#ifndef BACKEND_DB_ENTITY_BUILD_HPP__
#define BACKEND_DB_ENTITY_BUILD_HPP__

#include "backend/db/DSL.hpp"
#include "backend/db/PreparedStatement.hpp"
#include "backend/db/Queries.hpp"

#include <cstdint>
#include <string>

namespace backend::db::entity::build {
    using id = db::Column<"id", uint32_t>;
    using product_name = db::Column<"product_name", std::string>;
    using build_name = db::Column<"build_name", std::string>;
    using build_config = db::Column<"build_config", std::string>;
    using cdn_config = db::Column<"cdn_config", std::string>;
    using detected_at = db::Column<"detected_at", uint64_t>;
    using region = db::Column<"region", std::string>;

    using Entity = db::Entity<"builds", "public", id, product_name, build_name, build_config, cdn_config, detected_at, region>;

    namespace dto {
        namespace columns {
            using id_count = db::Alias<"id_count", db::Over<
                db::Count<id>, 
                db::PartitionBy<product_name>
            >>;
        }

        using BuildName = db::Projection<id, build_name>;
        using BuildStatistics = db::Projection<columns::id_count, build_name, detected_at, build_config, cdn_config>;
    }

    namespace queries {
        /**
         * Selects all records.
         */
        using Select = db::PreparedStatement<"Builds.Select", db::select::Query<
            Entity,
            db::From<Entity>
        >>;

        /**
         * Selects a build by its name (build-name key in build configuration) .
         */
        using SelByName = db::PreparedStatement<"Builds.SelByName", db::select::Query<
            Entity,
            db::From<Entity>,
            db::Where<
                db::And<
                    db::Equals<build_name>,
                    db::Equals<region>
                >
            >
        >>;

        /**
         * Selects a build by its ID (database PK).
         */
        using SelById = db::PreparedStatement<"Builds.SelById", db::select::Query<
            Entity,
            db::From<Entity>,
            db::Where<
                db::Equals<id>
            >
        >>;

        /**
         * Selects all builds for a given product name.
         */
        using SelByProduct = db::PreparedStatement<"Builds.SelByProduct", db::select::Query<
            dto::BuildName,
            db::From<Entity>,
            db::Where<
                db::Equals<product_name>
            >
        >>;

        /**
         * Selects the amount of builds and the name of the latest build for a specific product.
         */
        using SelStatistics = db::PreparedStatement<"Builds.SelStatistics", db::select::Query<
            dto::BuildStatistics,
            db::From<Entity>,
            db::Where<
                db::Equals<product_name>
            >,
            db::OrderBy<
                db::OrderByClause<false, id>
            >,
            db::Limit<1, 0>
        >>;
        
        using Insert = db::PreparedStatement<"Builds.Insert", db::insert::Query<
            Entity,
            region, product_name, build_name, build_config, cdn_config, detected_at
        >>;
    }
}

#endif // BACKEND_DB_ENTITY_BUILD_HPP__

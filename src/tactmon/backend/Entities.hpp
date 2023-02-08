#ifndef backend_entities_hpp__
#define backend_entities_hpp__

#include "backend/Queries.hpp"

#include <ext/Literal.hpp>

#include <cstdint>
#include <string>

namespace backend::db::entities {
    namespace build {
        using id           = db::Column<"id", uint32_t>;
        using build_name   = db::Column<"build_name", std::string>;
        using build_config = db::Column<"build_config", std::string>;
        using cdn_config   = db::Column<"cdn_config", std::string>;
        using product_name = db::Column<"product_name", std::string>;

        using Entity       = db::Entity<"builds", id, build_name, build_config, cdn_config, product_name>;

        namespace dto {
            using BuildName = db::Projection<id, build_name>;
        }

        namespace queries {
            using SelectByName = db::PreparedStatement<"SelectBuildByName", db::Select<
                Entity,
                db::Schema<"public", Entity>,
                db::Equals<build_name>,
                db::Limit<1, 0>
            >>;

            using SelectForProduct = db::PreparedStatement<"SelectBuildsForProduct", db::Select<
                dto::BuildName,
                db::Schema<"public", Entity>,
                db::Equals<product_name>,
                db::All
            >>;
        }
    }
}

#endif // backend_entities_hpp__

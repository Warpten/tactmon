#ifndef BACKEND_DB_ENTITY_PRODUCT_HPP__
#define BACKEND_DB_ENTITY_PRODUCT_HPP__

#include "backend/db/orm/Column.hpp"
#include "backend/db/orm/Entity.hpp"
#include "backend/db/orm/Shared.hpp"
#include "backend/db/orm/PreparedStatement.hpp"
#include "backend/db/orm/delete/Query.hpp"
#include "backend/db/orm/insert/Query.hpp"
#include "backend/db/orm/select/Query.hpp"
#include "backend/db/orm/update/Query.hpp"
#include "backend/db/orm/update/Set.hpp"

#include <cstdint>
#include <string>

namespace backend::db::entity::product {
    using id = db::Column<"id", uint32_t>;
    using name = db::Column<"name", std::string>;
    using sequence_id = db::Column<"sequence_id", uint64_t>;

    using Entity = db::Entity<"products", "public", id, name, sequence_id>;

    namespace queries {
        /**
         * Selects all records.
         */
        using Select = db::PreparedStatement<"Products.Select", db::select::Query<
            Entity::projection_type,
            Entity
        >>;

        /**
         * Selects a product by its name (as seen in Ribbit).
         */
        using SelByName = db::PreparedStatement<"Products.SelByName", db::select::Query<
            Entity::projection_type,
            Entity,
            db::Where<
                db::Equals<name, db::Parameter>
            >
        >>;

        /**
         * Selects a product by its ID (database PK).
         */
        using SelById = db::PreparedStatement<"Products.SelById", db::select::Query<
            Entity::projection_type,
            Entity,
            db::Where<
                db::Equals<id, db::Parameter>
            >
        >>;

        using Insert = db::PreparedStatement<"Products.Insert", db::insert::Query<
            Entity,
            name, sequence_id
        >>;

        using Update = db::PreparedStatement<"Products.Update", db::update::Query<
            Entity,
            db::update::Set<
                db::Equals<sequence_id, db::Parameter>
            >
        >::Where<
            db::Equals<name, db::Parameter>
        >>;
    }
}

#endif // BACKEND_DB_ENTITY_PRODUCT_HPP__

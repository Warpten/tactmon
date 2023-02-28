#ifndef BACKEND_DB_ENTITY_PRODUCT_HPP__
#define BACKEND_DB_ENTITY_PRODUCT_HPP__

#include "backend/db/DSL.hpp"
#include "backend/db/PreparedStatement.hpp"
#include "backend/db/Queries.hpp"

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
            Entity,
            db::From<Entity>
        >>;

        /**
         * Selects a product by its name (as seen in Ribbit).
         */
        using SelByName = db::PreparedStatement<"Products.SelByName", db::select::Query<
            Entity,
            db::From<Entity>,
            db::Where<
                db::Equals<name>
            >
        >>;

        /**
         * Selects a product by its ID (database PK).
         */
        using SelById = db::PreparedStatement<"Products.SelById", db::select::Query<
            Entity,
            db::From<Entity>,
            db::Where<
                db::Equals<id>
            >
        >>;

        using Insert = db::PreparedStatement<"Products.Insert", db::insert::Query<
            Entity,
            name, sequence_id
        >>;

        using Update = db::PreparedStatement<"Products.Update", db::update::Query<
            Entity,
            db::Where<
                db::Equals<name>
            >,
            sequence_id
        >>;
    }
}

#endif // BACKEND_DB_ENTITY_PRODUCT_HPP__

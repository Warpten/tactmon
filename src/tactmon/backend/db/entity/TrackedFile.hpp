#ifndef BACKEND_DB_ENTITY_TRAKCED_FILE_HPP__
#define BACKEND_DB_ENTITY_TRAKCED_FILE_HPP__

#include "backend/db/DSL.hpp"
#include "backend/db/PreparedStatement.hpp"
#include "backend/db/Queries.hpp"

#include <cstdint>
#include <string>

namespace backend::db::entity::tracked_file {
    using id = db::Column<"id", uint32_t>;
    using product_name = db::Column<"product_name", std::string>;
    using file_path = db::Column<"file_path", std::string>;
    using display_name = db::Column<"display_name", std::string>;

    using Entity = db::Entity<"tracked_files", "public", id, product_name, file_path, display_name>;

    namespace queries {
        /**
        * Selects all records.
        */
        using Select = db::PreparedStatement<"TrackedFiles.Select", db::select::Query<
            Entity,
            db::From<Entity>
        >>;

        using Insert = db::PreparedStatement<"TrackedFiles.Insert", db::insert::Query<
            Entity,
            Ignore,
            product_name, file_path, display_name
        >>;

        using Delete = db::PreparedStatement<"TrackedFiles.Delete", db::del::Query<
            Entity,
            db::Where<
                db::And<
                    db::Equals<product_name>,
                    db::Equals<file_path>
                >
            >
        >>;
    }
}

#endif // BACKEND_DB_ENTITY_TRAKCED_FILE_HPP__

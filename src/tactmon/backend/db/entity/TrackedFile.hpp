#ifndef BACKEND_DB_ENTITY_TRAKCED_FILE_HPP__
#define BACKEND_DB_ENTITY_TRAKCED_FILE_HPP__

#include "backend/db/orm/Column.hpp"
#include "backend/db/orm/Entity.hpp"
#include "backend/db/orm/Shared.hpp"
#include "backend/db/orm/PreparedStatement.hpp"
#include "backend/db/orm/delete/Query.hpp"
#include "backend/db/orm/insert/Query.hpp"
#include "backend/db/orm/select/Query.hpp"
#include "backend/db/orm/update/Query.hpp"

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
            Entity
        >>;

        using Insert = db::PreparedStatement<"TrackedFiles.Insert", db::insert::Query<
            Entity,
            insert::Value<product_name>, insert::Value<file_path>, insert::Value<display_name>
        >>;

        using Delete = db::PreparedStatement<"TrackedFiles.Delete", db::del::Query<
            Entity,
            db::Where<
                db::Conjunction<
                    db::Equals<product_name, db::Parameter>,
                    db::Equals<file_path, db::Parameter>
                >
            >
        >>;
    }
}

#endif // BACKEND_DB_ENTITY_TRAKCED_FILE_HPP__

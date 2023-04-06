#ifndef BACKEND_DB_ENTITY_COMMAND_STATE_HPP_
#define BACKEND_DB_ENTITY_COMMAND_STATE_HPP_

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

namespace backend::db::entity::command_state {
    using id = db::Column<"id", uint32_t>;
    using name = db::Column<"name", std::string>;
    using hash = db::Column<"hash", uint64_t>;
    using version = db::Column<"version", uint32_t>;

    using Entity = db::Entity<"command_states", "public", id, name, hash, version>;

    namespace queries {
        /**
        * Selects all records.
        */
        using Select = db::PreparedStatement<"CommandStates.Select", db::select::Query<
            Entity,
            Entity
        >>;

        using SelectByName = db::PreparedStatement<"CommandStates.SelectByName", db::select::Query<
            Entity,
            Entity,
            db::Where<
                db::Equals<name, db::Parameter>
            >
        >>;

        using InsertOrUpdate = db::PreparedStatement<"CommandStates.InsertOrUpdate", db::insert::Query<
            Entity,
            name, hash, version
        >::OnConflict<db::insert::UpdateFromExcluded<Entity, name, hash, version>>>;
    }
}

#endif // BACKEND_DB_ENTITY_COMMAND_STATE_HPP_

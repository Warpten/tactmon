#ifndef BACKEND_DB_ENTITY_BOUND_CHANNEL_HPP__
#define BACKEND_DB_ENTITY_BOUND_CHANNEL_HPP__

#include "backend/db/orm/Entity.hpp"
#include "backend/db/orm/Shared.hpp"
#include "backend/db/orm/PreparedStatement.hpp"
#include "backend/db/orm/delete/Query.hpp"
#include "backend/db/orm/insert/Query.hpp"
#include "backend/db/orm/select/Query.hpp"

#include <cstdint>
#include <string>

namespace backend::db::entity::bound_channel {
    using id = db::Column<"id", uint32_t>;
    using guild_id = db::Column<"guild_id", uint64_t>;
    using channel_id = db::Column<"channel_id", uint64_t>;
    using product_name = db::Column<"product_name", std::string>;

    using Entity = db::Entity<"bound_channels", "public", id, guild_id, channel_id, product_name>;

    namespace queries {
        /**
        * Selects all records.
        */
        using Select = db::PreparedStatement<"BoundChannels.Select", db::select::Query<
            Entity,
            Entity
        >>;

        using Insert = db::PreparedStatement<"BoundChannels.Insert", db::insert::Query<
            Entity,
            insert::Value<guild_id>, insert::Value<channel_id>, insert::Value<product_name>
        >>;

        using Delete = db::PreparedStatement<"BoundChannels.Delete", db::del::Query<
            Entity,
            db::Where<
                db::Conjunction<
                    db::Equals<channel_id, db::Parameter>,
                    db::Equals<product_name, db::Parameter>
                >
            >
        >>;
    }
}

#endif // BACKEND_DB_ENTITY_BOUND_CHANNEL_HPP__

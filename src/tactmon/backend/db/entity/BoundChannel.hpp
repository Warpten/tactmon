#ifndef BACKEND_DB_ENTITY_BOUND_CHANNEL_HPP__
#define BACKEND_DB_ENTITY_BOUND_CHANNEL_HPP__

#include "backend/db/DSL.hpp"
#include "backend/db/PreparedStatement.hpp"
#include "backend/db/Queries.hpp"

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
            db::From<Entity>
        >>;

        using Insert = db::PreparedStatement<"BoundChannels.Insert", db::insert::Query<
            Entity,
            guild_id, channel_id, product_name
        >>;

        using Delete = db::PreparedStatement<"BoundChannels.Delete", db::del::Query<
            Entity,
            db::Where<
                db::And<
                    db::Equals<channel_id>,
                    db::Equals<product_name>
                >
            >
        >>;
    }
}

#endif // BACKEND_DB_ENTITY_BOUND_CHANNEL_HPP__

#include "backend/db/entity/Build.hpp"
#include "backend/Product.hpp"
#include "frontend/commands/CacheStatusCommand.hpp"
#include "frontend/Discord.hpp"

#include <fmt/chrono.h>
#include <fmt/format.h>

namespace db = backend::db;
namespace entity = db::entity;
namespace build = entity::build;

namespace frontend::commands {
    dpp::slashcommand CacheStatusCommand::GetRegistrationInfo(dpp::cluster& bot) const {
        return dpp::slashcommand("cache-status", "Gets cache status (memory-loaded builds and expiry time).", bot.me.id);
    }

    bool CacheStatusCommand::Matches(dpp::interaction const& evnt) const {
        if (std::holds_alternative<dpp::autocomplete_interaction>(evnt.data))
            return evnt.get_autocomplete_interaction().name == "cache-status";

        return evnt.get_command_name() == "cache-status";
    }

    void CacheStatusCommand::HandleSlashCommand(dpp::slashcommand_t const& evnt, frontend::Discord& cluster) {
        dpp::embed responseEmbed;
        responseEmbed.set_color(0x0000FF00u)
            .set_title(fmt::format("{} builds loaded.", cluster.productManager.size()));

        cluster.productManager.ForEachProduct([&](backend::Product& product, std::chrono::high_resolution_clock::time_point expiryTime)
        {
            auto expiryDelta = std::chrono::duration_cast<std::chrono::seconds>(expiryTime - std::chrono::high_resolution_clock::now());
            if (expiryDelta.count() < 0) {
                responseEmbed.add_field("",
                    fmt::format("`{}`: Currently unloading...", db::get<build::build_name>(product.GetLoadedBuild())),
                    true
                );
            } else {
                responseEmbed.add_field("",
                    fmt::format("`{0}`: Unloads in **{1:%M}** minutes and **{1:%S}** seconds.",
                        db::get<build::build_name>(product.GetLoadedBuild()), expiryDelta),
                    true
                );
            }
        });

        evnt.edit_response(dpp::message().add_embed(responseEmbed));
    }
}

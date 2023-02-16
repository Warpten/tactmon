#include "backend/db/DSL.hpp"
#include "backend/db/entity/Build.hpp"
#include "backend/Product.hpp"
#include "frontend/commands/CacheStatusCommand.hpp"
#include "frontend/Discord.hpp"

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
            .set_title(std::format("{} builds loaded.", cluster.productManager.size()));

        cluster.productManager.ForEachProduct([&](backend::Product& product, std::chrono::high_resolution_clock::time_point expiryTime) {
            responseEmbed.add_field(
                db::get<build::build_name>(product.GetLoadedBuild()),
                std::format("Unloads in **{0:%M}** minutes and **{0:%S}** seconds.", std::chrono::duration_cast<std::chrono::seconds>(expiryTime - std::chrono::high_resolution_clock::now())),
                true
            );
        });

        evnt.edit_response(dpp::message().add_embed(responseEmbed));
    }

    void CacheStatusCommand::HandleAutoCompleteEvent(dpp::autocomplete_t const& evnt, frontend::Discord& cluster) {
        // This overload is technically not necessary for now (because the argument is declared as non-autocompletable).
    }
}

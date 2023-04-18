#include "backend/Database.hpp"
#include "backend/db/entity/Build.hpp"
#include "backend/db/repository/Build.hpp"
#include "frontend/commands/ProductStatusCommand.hpp"
#include "frontend/Discord.hpp"

#include <fmt/chrono.h>
#include <fmt/format.h>

namespace db = backend::db;
namespace entity = db::entity;
namespace build = entity::build;

namespace frontend::commands {
    dpp::slashcommand ProductStatusCommand::GetRegistrationInfo(dpp::cluster& bot) const {
        return dpp::slashcommand("status", "Gets the status of a specific product.", bot.me.id)
            .add_option(dpp::command_option(dpp::command_option_type::co_string, "product", "The product for which you want to download a file.", true)
                .set_auto_complete(false)
            );
    }

    bool ProductStatusCommand::Matches(dpp::interaction const& evnt) const {
        if (std::holds_alternative<dpp::autocomplete_interaction>(evnt.data))
            return evnt.get_autocomplete_interaction().name == "status";

        return evnt.get_command_name() == "status";
    }

    void ProductStatusCommand::HandleSlashCommand(dpp::slashcommand_t const& evnt, frontend::Discord& cluster) {
        std::string product = std::get<std::string>(evnt.get_parameter("product"));

        auto entity = cluster.db.builds->GetStatisticsForProduct(product);
        if (!entity.has_value()) {
            evnt.edit_response(fmt::format("No version found for the **{}** product.", product));

            return;
        }

        uint64_t seenTimestamp = db::get<build::detected_at>(*entity);
        std::chrono::system_clock::time_point timePoint{ std::chrono::seconds { seenTimestamp  } };

        evnt.edit_response(dpp::message().add_embed(
            dpp::embed()
                .set_color(0x0000FF00u)
                .set_title(fmt::format("Most recent build: {}.", db::get<build::build_name>(*entity)))
                .set_description(fmt::format("First seen on **{0:%D}** at **{0:%r}**.", timePoint))
                .add_field("build_config", fmt::format("`{}`", db::get<build::build_config>(*entity)), true)
                .add_field("cdn_config", fmt::format("`{}`", db::get<build::cdn_config>(*entity)), true)
                .set_footer(dpp::embed_footer()
                    .set_text(
                        fmt::format("{} known builds for product {}.", db::get<build::dto::columns::id_count>(*entity), product)
                    )
                )
        ));
    }
}

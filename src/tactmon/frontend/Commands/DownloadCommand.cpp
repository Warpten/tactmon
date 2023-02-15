#include "frontend/commands/DownloadCommand.hpp"
#include "frontend/Discord.hpp"

#include "backend/db/DSL.hpp"
#include "backend/db/entity/Build.hpp"
#include "backend/db/repository/Build.hpp"

#include <tact/data/FileLocation.hpp>

namespace db = backend::db;
namespace entity = db::entity;
namespace build = entity::build;

namespace frontend::commands {
    dpp::slashcommand DownloadCommand::GetRegistrationInfo(dpp::cluster& bot) const {
        return dpp::slashcommand("download", "Downloads a specific file for a game build.", bot.me.id)
            .add_option(dpp::command_option(dpp::command_option_type::co_string, "product", "The product for which you want to download a file.", true)
                .set_auto_complete(false)
            ).add_option(dpp::command_option(dpp::command_option_type::co_string, "version", "The version of the game for which you want to download a file.", true)
                .set_auto_complete(true)
            ).add_option(dpp::command_option(dpp::command_option_type::co_string, "file", "Complete path to the file you want to download.", true));
    }

    bool DownloadCommand::Matches(dpp::interaction const& evnt) const {
        if (std::holds_alternative<dpp::autocomplete_interaction>(evnt.data))
            return evnt.get_autocomplete_interaction().name == "download";

        return evnt.get_command_name() == "download";
    }

    void DownloadCommand::HandleSlashCommand(dpp::slashcommand_t const& evnt, frontend::Discord& cluster) {
        std::string product = std::get<std::string>(evnt.get_parameter("product"));
        std::string version = std::get<std::string>(evnt.get_parameter("version"));
        std::string file = std::get<std::string>(evnt.get_parameter("file"));

        auto buildEntry = cluster.db.builds.GetByBuildName(version);
        if (!buildEntry.has_value()) {
            evnt.edit_response(dpp::message().add_embed(
                dpp::embed()
                .set_title(product)
                .set_color(0x00FF0000u)
                .set_description("Build configuration cannot be found.")
            ));

            return;
        }

        // 1. If user-provided product and database-backed product don't match, error out
        if (product != db::get<build::product_name>(*buildEntry)) {
            evnt.edit_response(dpp::message().add_embed(
                dpp::embed()
                    .set_title(product)
                    .set_color(0x00FF0000u)
                    .set_description("Build configuration cannot be found for this product.")
            ));

            return;
        }

        evnt.edit_response(dpp::message().add_embed(
            dpp::embed()
                .set_title(version)
                .set_description("Loading... This may take a while.")
                .set_color(0x0000FF00u)
        ));

        // 2. Obtain an instance of the product.
        cluster.productManager.LoadConfiguration(product, *buildEntry, [=](backend::Product& productHandler) {
            // 3. Locate the file
            std::optional<tact::data::FileLocation> fileLocation = productHandler->FindFile(file);
            if (!fileLocation.has_value()) {
                evnt.edit_response(dpp::message().add_embed(
                    dpp::embed()
                        .set_title(product)
                        .set_color(0x00FF0000u)
                        .set_description(std::format("`{}` does not exist.", file))
                        .set_footer(dpp::embed_footer()
                            .set_text(version))
                ));

                return;
            }

            // 4. Generate download link
            // Generate a link to the http server
            // The server will perform routing over all available CDN servers for the provided build
            // and stream the response to the client.
            evnt.edit_response(dpp::message().add_embed(
                dpp::embed()
                .set_title("Download this file.")
                .set_url("http://www.google.fr") // TODO: Generate link
                .set_description(std::format("Click here to download `{}`", file))
                .set_footer(dpp::embed_footer()
                    .set_text(version))
            ));
        });
    }

    void DownloadCommand::HandleAutoCompleteEvent(dpp::autocomplete_t const& evnt, frontend::Discord& cluster) {
        std::optional<std::string> productName = std::nullopt;

        for (dpp::command_option const& eventOption : evnt.options) {
            // If the command allows for a product, use that as a filter for version selection
            if (eventOption.name == "product")
                productName = std::get<std::string>(eventOption.value);

            if (!eventOption.focused)
                continue;

            if (eventOption.name == "version") {
                std::string optionValue = std::get<std::string>(eventOption.value);

                // Don't suggest on empty value (would explode)
                if (optionValue.empty())
                    break;

                size_t selectedValueCount = 0;

                dpp::interaction_response interactionResponse{ dpp::ir_autocomplete_reply };
                cluster.db.builds.WithValues([&](auto entries) {
                    for (entity::build::Entity::as_projection const& entry : entries) {
                        std::string buildName = db::get<build::build_name>(entry);
                        if (buildName.find(optionValue) == std::string::npos || (productName.has_value() && db::get<build::product_name>(entry) != productName))
                            continue;

                        interactionResponse.add_autocomplete_choice(dpp::command_option_choice(buildName, buildName));
                        // Limit to 100 values
                        if (selectedValueCount++ >= 100)
                            break;
                    }
                });

                cluster.bot.interaction_response_create_sync(evnt.command.id, evnt.command.token, interactionResponse);
                break;
            }
        }
    }
}

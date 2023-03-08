#include "backend/Database.hpp"
#include "backend/db/entity/Product.hpp"
#include "backend/db/entity/TrackedFile.hpp"
#include "backend/db/repository/Product.hpp"
#include "backend/db/repository/TrackedFile.hpp"
#include "frontend/commands/TrackFileCommand.hpp"
#include "frontend/Discord.hpp"

#include <fmt/chrono.h>
#include <fmt/format.h>

namespace db = backend::db;
namespace entity = db::entity;
namespace tracked_file = entity::tracked_file;
namespace product = entity::product;

namespace frontend::commands {
    dpp::slashcommand TrackFileCommand::GetRegistrationInfo(dpp::cluster& bot) const {
        return dpp::slashcommand("track", "Adds or removes a file from the tracked files for a specific product.", bot.me.id)
            .add_option(
                dpp::command_option(dpp::co_sub_command_group, "filename", "Adds or remove a file from the tracked files for a product via its name.")
                    .add_option(
                        dpp::command_option(dpp::co_sub_command, "add", "Tracks a specific file for a given product.")
                            .add_option(dpp::command_option(dpp::co_string, "product", "The name of the product.", true).set_auto_complete(true))
                            .add_option(dpp::command_option(dpp::co_string, "filepath", "Absolute path to the file.", true))
                    )
                    .add_option(
                        dpp::command_option(dpp::co_sub_command, "remove", "Removes a file from the list of track files for a specific product.")
                            .add_option(dpp::command_option(dpp::co_string, "product", "The name of the product.", true).set_auto_complete(true))
                            .add_option(dpp::command_option(dpp::co_string, "filepath", "Absolute path to the file.", true).set_auto_complete(true))
                    )
            );
    }

    bool TrackFileCommand::Matches(dpp::interaction const& evnt) const {
        if (std::holds_alternative<dpp::autocomplete_interaction>(evnt.data))
            return evnt.get_autocomplete_interaction().name == "track";

        return evnt.get_command_name() == "track";
    }

    void TrackFileCommand::HandleSlashCommand(dpp::slashcommand_t const& evnt, frontend::Discord& cluster) {
        dpp::command_interaction interaction = evnt.command.get_command_interaction();
        dpp::command_data_option const& subcommand = interaction.options[0];

        std::string product = std::get<std::string>(evnt.get_parameter("product"));
        std::string filePath = std::get<std::string>(evnt.get_parameter("filepath"));

        bool alreadyChecked = cluster.db.trackedFiles.Any([&](auto const& entry) {
            return db::get<tracked_file::product_name>(entry) == product
                && db::get<tracked_file::file_path>(entry) == filePath;
        });

        if (subcommand.name == "add") {
            if (alreadyChecked) {
                evnt.edit_response(dpp::message().add_embed(
                    dpp::embed()
                        .set_description(fmt::format("File `{}` is already tracked for product `{}`.", filePath, product))
                ));
            }
            else {
                cluster.db.trackedFiles.Insert(product, filePath);
                evnt.edit_response(dpp::message().add_embed(
                    dpp::embed()
                        .set_description(fmt::format("File `{}` is now tracked for product `{}`.", filePath, product))
                        .set_footer("This may take a minute to apply.", "")
                ));
            }
        }
        else if (subcommand.name == "remove") {
            if (alreadyChecked) {
                cluster.db.trackedFiles.Delete(product, filePath);
                evnt.edit_response(dpp::message().add_embed(
                    dpp::embed()
                        .set_description(fmt::format("File `{}` is no longer tracked for product `{}`.", filePath, product))
                        .set_footer("This may take a minute to apply.", "")
                ));
            }
            else {
                evnt.edit_response(dpp::message().add_embed(
                    dpp::embed()
                        .set_description(fmt::format("File `{}` is not currently tracked for product `{}`. Did you forget to track it?", filePath, product))
                ));
            }
        }
    }
}

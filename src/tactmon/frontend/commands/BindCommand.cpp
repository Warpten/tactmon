#include "backend/Database.hpp"
#include "backend/db/repository/BoundChannel.hpp"
#include "frontend/commands/BindCommand.hpp"
#include "frontend/Discord.hpp"

#include <fmt/chrono.h>
#include <fmt/format.h>

namespace db = backend::db;
namespace entity = db::entity;
namespace build = entity::build;

namespace frontend::commands {
    dpp::slashcommand BindCommand::GetRegistrationInfo(dpp::cluster& bot) const {
        return dpp::slashcommand("bind", "Toggles Ribbit push announcements to this channel.", bot.me.id)
            .add_option(dpp::command_option(dpp::command_option_type::co_string, "product", "The product to bind to this channel.", true)
                .set_auto_complete(false)
            ).add_option(dpp::command_option(dpp::command_option_type::co_boolean, "bind", "Bind or unbind from this channel.", true)
                .set_auto_complete(false)
            ).set_default_permissions(0);
    }

    bool BindCommand::Matches(dpp::interaction const& evnt) const {
        if (std::holds_alternative<dpp::autocomplete_interaction>(evnt.data))
            return evnt.get_autocomplete_interaction().name == "bind";

        return evnt.get_command_name() == "bind";
    }

    void BindCommand::HandleSlashCommand(dpp::slashcommand_t const& evnt, frontend::Discord& cluster) {
        std::string product = std::get<std::string>(evnt.get_parameter("product"));
        bool bind = std::get<bool>(evnt.get_parameter("bind"));

        if (!bind) {
            evnt.edit_response(fmt::format("I will no longer announce pushes to `{}` on this channel."
                "\r\n:information_source: This may take a minute to apply.", product));

            cluster.db.boundChannels->Delete(static_cast<uint64_t>(evnt.command.channel_id), std::move(product));
        } else {
            evnt.edit_response(fmt::format("I will announce pushes to `{}` on this channel."
                "\r\n:information_source: This may take a minute to apply.", product));

            cluster.db.boundChannels->Insert(static_cast<uint64_t>(evnt.command.guild_id), static_cast<uint64_t>(evnt.command.channel_id), std::move(product));
        }
    }
}

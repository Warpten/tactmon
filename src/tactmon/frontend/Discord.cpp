#include "frontend/commands/BindCommand.hpp"
#include "frontend/commands/CacheStatusCommand.hpp"
#include "frontend/commands/DownloadCommand.hpp"
#include "frontend/commands/ProductStatusCommand.hpp"
#include "frontend/commands/TrackFileCommand.hpp"
#include "frontend/Discord.hpp"
#include "backend/db/entity/Build.hpp"
#include "backend/db/repository/Build.hpp"
#include "utility/Logging.hpp"

#include "utility/Literal.hpp"

#include <cstdint>
#include <functional>
#include <string>
#include <string_view>
#include <tuple>
#include <type_traits>

#include <spdlog/spdlog.h>

#include <boost/algorithm/string.hpp>
#include <boost/range/adaptor/filtered.hpp>

#include <fmt/format.h>

namespace frontend {
    namespace db = backend::db;
    namespace entity = db::entity;
    namespace build = entity::build;

    Discord::Discord(size_t threadCount, std::string const& token,
        backend::ProductCache& productManager, backend::Database& database, std::shared_ptr<net::Server> proxyServer)
        : _threadPool(threadCount), httpServer(proxyServer), productManager(productManager), db(database), bot(token)
    {
        _logger = utility::logging::GetLogger("discord");

        bot.on_ready(std::bind(&Discord::HandleReady, this, std::placeholders::_1));
        bot.on_guild_create(std::bind(&Discord::HandleGuildCreate, this, std::placeholders::_1));
        bot.on_slashcommand(std::bind(&Discord::HandleSlashCommand, this, std::placeholders::_1));
        bot.on_form_submit(std::bind(&Discord::HandleFormSubmitEvent, this, std::placeholders::_1));
        bot.on_select_click(std::bind(&Discord::HandleSelectClickEvent, this, std::placeholders::_1));
        bot.on_log(std::bind(&Discord::HandleLogEvent, this, std::placeholders::_1));
        bot.on_autocomplete(std::bind(&Discord::HandleAutoCompleteEvent, this, std::placeholders::_1));
    }

    Discord::~Discord() {
        bot.shutdown();
    }

    void Discord::Run() {
        bot.start(dpp::start_type::st_return);

        if (httpServer != nullptr)
            RegisterCommand<frontend::commands::DownloadCommand>();

        RegisterCommand<frontend::commands::ProductStatusCommand>();
        RegisterCommand<frontend::commands::CacheStatusCommand>();
        RegisterCommand<frontend::commands::BindCommand>();
        RegisterCommand<frontend::commands::TrackFileCommand>();
    }

    void Discord::HandleReady(dpp::ready_t const& evnt) {
        // Housekeeping: delete any global command.
        bot.global_bulk_command_create({ });
    }

    void Discord::HandleGuildCreate(dpp::guild_create_t const& evnt) {
        if (evnt.created == nullptr)
            return;

        std::vector<dpp::slashcommand> commands;
        for (std::shared_ptr<commands::ICommand> command : _commands)
            commands.push_back(command->GetRegistrationInfo(bot));

        bot.guild_bulk_command_create(commands, evnt.created->id);
    }

    void Discord::HandleLogEvent(dpp::log_t const& evnt) {
        switch (evnt.severity) {
            case dpp::ll_trace:   _logger->trace("{}", evnt.message); break;
            case dpp::ll_debug:   _logger->debug("{}", evnt.message); break;
            case dpp::ll_info:    _logger->info("{}", evnt.message); break;
            case dpp::ll_warning: _logger->warn("{}", evnt.message); break;
            case dpp::ll_error:   _logger->error("{}", evnt.message); break;
            default:              _logger->critical("{}", evnt.message); break;
        }
    }

    void Discord::HandleSlashCommand(dpp::slashcommand_t const& evnt) {
        try {
            evnt.thinking(false);

            for (std::shared_ptr<frontend::commands::ICommand> command : _commands)
                if (command->OnSlashCommand(evnt, *this))
                    return;

            evnt.reply(dpp::message().add_embed(
                dpp::embed()
                    .set_color(0x00FF0000u)
                    .set_title("An error occured.")
                    .set_description("No command handler found for this command.")
                    .set_footer(dpp::embed_footer()
                    .set_text("How the hell did you manage to trigger this?")
                )
            ));
        } catch (std::exception const& ex) {
            _logger->error(ex.what());
        }
    }

    void Discord::HandleAutoCompleteEvent(dpp::autocomplete_t const& evnt) {
        try {
            for (std::shared_ptr<frontend::commands::ICommand> command : _commands) {
                if (command->OnAutoComplete(evnt, *this))
                    return;
            }

            // Empty response by default.
            bot.interaction_response_create_sync(evnt.command.id, evnt.command.token, dpp::interaction_response{ dpp::ir_autocomplete_reply });
        } catch (std::exception const& ex) {
            _logger->error(ex.what());
        }
    }

    void Discord::HandleSelectClickEvent(dpp::select_click_t const& evnt) {
        try {
            for (std::shared_ptr<frontend::commands::ICommand> command : _commands) {
                if (command->OnSelectClick(evnt, *this))
                    return;
            }
        } catch (std::exception const& ex) {
            _logger->error(ex.what());
        }
    }

    void Discord::HandleFormSubmitEvent(dpp::form_submit_t const& evnt) { }
}

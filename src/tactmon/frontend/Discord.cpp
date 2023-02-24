#include "frontend/commands/CacheStatusCommand.hpp"
#include "frontend/commands/DownloadCommand.hpp"
#include "frontend/commands/ProductStatusCommand.hpp"
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

    Discord::Discord(boost::asio::io_context::strand strand, std::string_view token,
        backend::ProductCache& productManager, backend::Database& database, frontend::Tunnel& httpServer)
        : httpServer(httpServer), productManager(productManager), db(database), bot(std::string{ token }), _strand(strand)
    {
        _logger = utility::logging::GetLogger("discord");

        bot.on_ready(std::bind(&Discord::HandleReadyEvent, this, std::placeholders::_1));
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
    }

    void Discord::HandleLogEvent(dpp::log_t const& event) {
        switch (event.severity) {
            case dpp::ll_trace:   _logger->trace("{}", event.message); break;
            case dpp::ll_debug:   _logger->debug("{}", event.message); break;
            case dpp::ll_info:    _logger->info("{}", event.message); break;
            case dpp::ll_warning: _logger->warn("{}", event.message); break;
            case dpp::ll_error:   _logger->error("{}", event.message); break;
            default:              _logger->critical("{}", event.message); break;
        }
    }

    void Discord::HandleReadyEvent(dpp::ready_t const& event) {
        if (!dpp::run_once<struct command_registration_handler>())
            return;

        RegisterCommand<frontend::commands::DownloadCommand>();
        RegisterCommand<frontend::commands::ProductStatusCommand>();
        RegisterCommand<frontend::commands::CacheStatusCommand>();
    }

    void Discord::HandleSlashCommand(dpp::slashcommand_t const& evnt) {
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
    }

    void Discord::HandleAutoCompleteEvent(dpp::autocomplete_t const& evnt) {
        for (std::shared_ptr<frontend::commands::ICommand> command : _commands) {
            if (command->OnAutoComplete(evnt, *this))
                return;
        }

        // Empty response by default.
        bot.interaction_response_create_sync(evnt.command.id, evnt.command.token, dpp::interaction_response { dpp::ir_autocomplete_reply });
    }

    void Discord::HandleSelectClickEvent(dpp::select_click_t const& evnt) {
        for (std::shared_ptr<frontend::commands::ICommand> command : _commands) {
            if (command->OnSelectClick(evnt, *this))
                return;
        }
    }

    void Discord::HandleFormSubmitEvent(dpp::form_submit_t const& evnt) { }
}

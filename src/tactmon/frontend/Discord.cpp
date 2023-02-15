#include "frontend/Commands/DownloadCommand.hpp"
#include "frontend/Discord.hpp"
#include "backend/db/entity/Build.hpp"
#include "backend/db/repository/Build.hpp"

#include <ext/Literal.hpp>
#include <logging/Sinks.hpp>

#include <cstdint>
#include <format>
#include <functional>
#include <string>
#include <string_view>
#include <tuple>
#include <type_traits>

#include <spdlog/spdlog.h>

#include <boost/algorithm/string.hpp>
#include <boost/range/adaptor/filtered.hpp>

namespace frontend {
    namespace db = backend::db;
    namespace entity = db::entity;
    namespace build = entity::build;

    Discord::Discord(boost::asio::io_context::strand strand, std::string_view token,
        backend::ProductCache& productManager, backend::Database& database)
        : bot(std::string{ token }), productManager(productManager), database(database), _strand(strand)
    {
        _logger = logging::GetLogger("discord");

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
    }

    void Discord::HandleSlashCommand(dpp::slashcommand_t const& evnt) {
        evnt.thinking(false);

        for (std::shared_ptr<frontend::commands::ICommand> command : _commands) {
            if (command->OnSlashCommand(evnt, *this))
                return;
        }

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

    void Discord::HandleFormSubmitEvent(dpp::form_submit_t const& event) {
    }

    void Discord::OnListProductCommand(dpp::slashcommand_t const& event, std::string const& product) {
        auto entity = database.GetBuildRepository().GetStatisticsForProduct(product);
        if (!entity.has_value()) {
            event.reply(std::format("No version found for the **{}** product.", product));

            return;
        }

        uint64_t seenTimestamp = db::get<build::detected_at>(*entity);
        std::chrono::system_clock::time_point timePoint{ std::chrono::seconds { seenTimestamp  }  };

        event.reply(dpp::message().add_embed(
            dpp::embed()
                .set_color(0x0000FF00u)
                .set_title(
                    std::format("Most recent build: {}.", db::get<build::build_name>(*entity))
                ).set_description(
                    std::format("First seen on **{:%D}** at **{:%r}**.", timePoint, timePoint)
                ).add_field("build_config", std::format("`{}`", db::get<build::build_config>(*entity)), true)
                .add_field("cdn_config", std::format("`{}`", db::get<build::cdn_config>(*entity)), true)
                .set_footer(dpp::embed_footer()
                    .set_text(
                        std::format("{} known builds for product {}.", db::get<build::dto::columns::id_count>(*entity), product)
                    )
                )
        ));
    }
}

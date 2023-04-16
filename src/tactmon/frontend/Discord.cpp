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

#include <boost/functional/hash.hpp>
#include <boost/range/adaptor/filtered.hpp>
#include <utility>

#include <fmt/format.h>

namespace frontend {
    namespace db = backend::db;
    namespace entity = db::entity;
    namespace build = entity::build;
    namespace command_state = entity::command_state;

    Discord::Discord(std::size_t threadCount, std::string const& token,
        backend::ProductCache& productManager, backend::Database& database, std::shared_ptr<net::Server> proxyServer)
        : _threadPool(threadCount), httpServer(std::move(proxyServer)), productManager(productManager), db(database), bot(token)
    {
        logger = utility::logging::GetLogger("discord");

        bot.on_ready([this](const dpp::ready_t& evnt) {
            this->HandleReady(evnt);
        });
        bot.on_guild_create([this](const dpp::guild_create_t& evnt) {
            this->HandleGuildCreate(evnt);
        });
        bot.on_slashcommand([this](const dpp::slashcommand_t& evnt) {
            this->HandleSlashCommand(evnt);
        });
        bot.on_form_submit([this](const dpp::form_submit_t& evnt) {
            this->HandleFormSubmitEvent(evnt);
        });
        bot.on_select_click([this](const dpp::select_click_t& evnt) {
            this->HandleSelectClickEvent(evnt);
        });
        bot.on_log([this](const dpp::log_t& evnt ) {
            this->HandleLogEvent(evnt);
        });
        bot.on_autocomplete([this](const dpp::autocomplete_t& evnt) {
            this->HandleAutoCompleteEvent(evnt);
        });
    }

    Discord::~Discord() {
        bot.shutdown();
    }

    /* static */ std::size_t Discord::Hash(dpp::slashcommand const& command) {
        std::size_t hash = 0;
        boost::hash_combine(hash, command.name);
        boost::hash_combine(hash, command.description);
        auto hashOption = [&hash](dpp::command_option const& opt, auto self) -> void {
            boost::hash_combine(hash, opt.name);
            boost::hash_combine(hash, opt.type);
            boost::hash_combine(hash, opt.description);
            boost::hash_combine(hash, opt.required);
            boost::hash_combine(hash, opt.autocomplete);

            for (dpp::command_option const& subOption : opt.options)
                self(subOption, self);
        };

        for (dpp::command_option const& option : command.options)
            hashOption(option, hashOption);

        return hash;
    }

    std::tuple<bool, std::size_t, uint32_t> Discord::RequiresSynchronization(dpp::slashcommand const& command) const {
        std::optional<command_state::Entity> databaseRecord = db.commandStates.FindCommand(command.name);

        // Compute the command's unversioned hash.
        std::size_t baseHash = Hash(command);

        // If the command doesn't have a record, it's a new one; always update and publish to Discord's backend.
        if (!databaseRecord.has_value())
            return std::tuple { true, baseHash, 1 };

        // Retrieve the command version and append it to the hash function.
        uint32_t commandVersion = db::get<command_state::version>(databaseRecord.value());

        std::size_t versionedHash = baseHash;
        boost::hash_combine(versionedHash, commandVersion);

        // And finally return relevant information.
        return std::tuple {
            static_cast<uint64_t>(versionedHash) != db::get<command_state::hash>(databaseRecord.value()),
            baseHash,
            commandVersion + 1
        };
    }

    void Discord::Run() {
        bot.start(dpp::start_type::st_return);
    }

    void Discord::HandleReady(dpp::ready_t const& evnt) {
        // Housekeeping: delete any global command.
        bot.global_bulk_command_create({ });

        if (httpServer != nullptr)
            RegisterCommand<frontend::commands::DownloadCommand>();

        RegisterCommand<frontend::commands::ProductStatusCommand>();
        RegisterCommand<frontend::commands::CacheStatusCommand>();
        RegisterCommand<frontend::commands::BindCommand>();
        RegisterCommand<frontend::commands::TrackFileCommand>();
    }

    void Discord::HandleGuildCreate(dpp::guild_create_t const& evnt) {
        if (evnt.created == nullptr)
            return;

        // std::vector<dpp::slashcommand> commands;
        for (auto&& command : _commands) {
            dpp::slashcommand registrationInfo = command->GetRegistrationInfo(bot);
            auto [needsUpdate, hash, newVersion] = RequiresSynchronization(registrationInfo);
            if (needsUpdate) {
                // Compute final updated hash and update the database with it.
                boost::hash_combine(hash, newVersion);
                db.commandStates.InsertOrUpdate(registrationInfo.name, hash, newVersion);

                // commands.emplace_back(std::move(registrationInfo));
                bot.guild_command_create(registrationInfo, evnt.created->id);
            }
        }

        // bot.guild_bulk_command_create(commands, evnt.created->id);
    }

    void Discord::HandleLogEvent(dpp::log_t const& evnt) {
        switch (evnt.severity) {
            case dpp::ll_trace:   logger->trace("{}", evnt.message); break;
            case dpp::ll_debug:   logger->debug("{}", evnt.message); break;
            case dpp::ll_info:    logger->info("{}", evnt.message); break;
            case dpp::ll_warning: logger->warn("{}", evnt.message); break;
            case dpp::ll_error:   logger->error("{}", evnt.message); break;
            default:              logger->critical("{}", evnt.message); break;
        }
    }

    void Discord::HandleSlashCommand(dpp::slashcommand_t const& evnt) {
        try {
            evnt.thinking(false);

            for (auto&& command : _commands)
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
            logger->error(ex.what());
        }
    }

    void Discord::HandleAutoCompleteEvent(dpp::autocomplete_t const& evnt) {
        try {
            for (auto&& command : _commands) {
                if (command->OnAutoComplete(evnt, *this))
                    return;
            }

            // Empty response by default.
            bot.interaction_response_create_sync(evnt.command.id, evnt.command.token, dpp::interaction_response { dpp::ir_autocomplete_reply });
        } catch (std::exception const& ex) {
            logger->error(ex.what());
        }
    }

    void Discord::HandleSelectClickEvent(dpp::select_click_t const& evnt) {
        try {
            for (auto&& command : _commands) {
                if (command->OnSelectClick(evnt, *this))
                    return;
            }
        } catch (std::exception const& ex) {
            logger->error(ex.what());
        }
    }

    void Discord::HandleFormSubmitEvent(dpp::form_submit_t const& evnt) { }
}

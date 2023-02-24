#pragma once

#include "backend/Database.hpp"
#include "backend/Product.hpp"
#include "frontend/commands/ICommand.hpp"
#include "frontend/Tunnel.hpp"

#include <memory>
#include <string>
#include <string_view>

#include <dpp/dpp.h>
#include <spdlog/logger.h>

#include <boost/asio/io_context_strand.hpp>
#include <boost/asio/post.hpp>

namespace frontend {
    struct Discord final {
        friend struct commands::ICommand;

        Discord(boost::asio::io_context::strand strand, std::string_view token,
            backend::ProductCache& manager, backend::Database& database, frontend::Tunnel& httpServer);
        ~Discord();

        void Run();

    private:
        template <typename T>
        void RegisterCommand() {
            auto command = _commands.emplace_back(std::make_shared<T>());
            bot.guild_command_create_sync(command->GetRegistrationInfo(bot), 377185808719020033);
        }

        void HandleReadyEvent(dpp::ready_t const& event);
        void HandleSlashCommand(dpp::slashcommand_t const& event);
        void HandleFormSubmitEvent(dpp::form_submit_t const& event);
        void HandleLogEvent(dpp::log_t const& event);
        void HandleSelectClickEvent(dpp::select_click_t const& event);
        void HandleAutoCompleteEvent(dpp::autocomplete_t const& event);

    private:
        template <typename T>
        void RunAsync(T&& value) {
            boost::asio::post(_strand, value);
        }

    public:
        frontend::Tunnel& httpServer;
        backend::ProductCache& productManager;
        backend::Database& db;
        dpp::cluster bot;

    private:
        boost::asio::io_context::strand _strand;
        std::shared_ptr<spdlog::logger> _logger;

        std::vector<std::shared_ptr<frontend::commands::ICommand>> _commands;
    };
}

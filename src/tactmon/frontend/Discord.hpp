#pragma once

#include "backend/Database.hpp"
#include "backend/Product.hpp"
#include "backend/ProductCache.hpp"
#include "frontend/commands/ICommand.hpp"
#include "net/Server.hpp"

#include <memory>
#include <string>
#include <string_view>

#include <boost/asio/io_context_strand.hpp>
#include <boost/asio/post.hpp>

#include <dpp/dpp.h>

#include <spdlog/logger.h>

namespace frontend {
    struct Discord final {
        friend struct commands::ICommand;

        Discord(size_t threadCount, std::string const& token,
            backend::ProductCache& manager, backend::Database& database, std::shared_ptr<net::Server> proxyServer);
        ~Discord();

        void Run();

    private:
        template <typename T>
        void RegisterCommand(dpp::snowflake const& guildID) {
            auto command = _commands.emplace_back(std::make_shared<T>());
            bot.guild_command_create(command->GetRegistrationInfo(bot), guildID);
        }

        void HandleGuildCreate(dpp::guild_create_t const& event);
        void HandleSlashCommand(dpp::slashcommand_t const& event);
        void HandleFormSubmitEvent(dpp::form_submit_t const& event);
        void HandleLogEvent(dpp::log_t const& event);
        void HandleSelectClickEvent(dpp::select_click_t const& event);
        void HandleAutoCompleteEvent(dpp::autocomplete_t const& event);

    private:
        template <typename T>
        void RunAsync(T&& value) {
            _threadPool.PostWork(value);
        }

        utility::ThreadPool _threadPool;
        std::shared_ptr<spdlog::logger> _logger;
        std::vector<std::shared_ptr<frontend::commands::ICommand>> _commands;

    public:
        std::shared_ptr<net::Server> httpServer;
        backend::ProductCache& productManager;
        backend::Database& db;
        dpp::cluster bot;
    };
}

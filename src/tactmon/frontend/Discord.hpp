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
        template <typename T, typename... Args>
        auto RegisterCommand()
            -> std::enable_if_t<std::is_constructible_v<T, Args...>>
        {
            _commands.emplace_back(std::make_shared<T>());
        }

        void HandleReady(dpp::ready_t const& evnt);
        void HandleGuildCreate(dpp::guild_create_t const& evnt);
        void HandleSlashCommand(dpp::slashcommand_t const& evnt);
        void HandleFormSubmitEvent(dpp::form_submit_t const& evnt);
        void HandleLogEvent(dpp::log_t const& evnt);
        void HandleSelectClickEvent(dpp::select_click_t const& evnt);
        void HandleAutoCompleteEvent(dpp::autocomplete_t const& evnt);

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

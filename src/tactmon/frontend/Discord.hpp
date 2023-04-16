#pragma once

#include "backend/Database.hpp"
#include "backend/Product.hpp"
#include "backend/ProductCache.hpp"
#include "frontend/commands/ICommand.hpp"
#include "net/Server.hpp"

#include <cstdint>
#include <memory>
#include <string>
#include <string_view>
#include <tuple>

#include <boost/asio/io_context_strand.hpp>
#include <boost/asio/post.hpp>

#include <dpp/dpp.h>

#include <spdlog/logger.h>

namespace frontend {
    struct Discord final {
        friend struct commands::ICommand;

        Discord(std::size_t threadCount, std::string const& token,
            backend::ProductCache& manager, backend::Database& database, std::shared_ptr<net::Server> proxyServer);
        ~Discord();

        void Run();

    private:
        template <typename T, typename... Args>
        auto RegisterCommand()
            -> std::enable_if_t<std::is_constructible_v<T, Args...>>
        {
            _commands.emplace_back(std::make_unique<T>());
        }

        void HandleReady(dpp::ready_t const& evnt);
        void HandleGuildCreate(dpp::guild_create_t const& evnt);
        void HandleSlashCommand(dpp::slashcommand_t const& evnt);
        void HandleFormSubmitEvent(dpp::form_submit_t const& evnt);
        void HandleLogEvent(dpp::log_t const& evnt);
        void HandleSelectClickEvent(dpp::select_click_t const& evnt);
        void HandleAutoCompleteEvent(dpp::autocomplete_t const& evnt);

    private:
        static std::size_t Hash(dpp::slashcommand const& command) ;

        /**
         * Returns a tuple describing if and how a command should be resynchronized with Discord.
         *
         * @returns A tuple for which values are the following:
         *          [0] Indicates if the command requires resynchronization with Discord's servers.
         *          [1] The command's new versioned hash.
         *          [2] The command's new version.
         */
        std::tuple<bool, std::size_t, uint32_t> RequiresSynchronization(dpp::slashcommand const& command) const;

        template <typename T>
        void RunAsync(T&& value) {
            _threadPool.PostWork(value);
        }

        utility::ThreadPool _threadPool;
        std::shared_ptr<spdlog::logger> _logger;
        std::vector<std::unique_ptr<frontend::commands::ICommand>> _commands;

    public:
        std::shared_ptr<spdlog::logger> logger;
        std::shared_ptr<net::Server> httpServer;
        backend::ProductCache& productManager;
        backend::Database& db;
        dpp::cluster bot;
    };
}

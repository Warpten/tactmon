#pragma once

#include "backend/Database.hpp"

#include <memory>
#include <string>
#include <string_view>

#include <dpp/dpp.h>
#include <spdlog/logger.h>

namespace tact::data::product {
    struct Manager;
}

namespace frontend {
    struct Discord {
        Discord(std::string_view token, tact::data::product::Manager& manager, backend::Database&& database);

        void Run();

    private:
        void HandleReadyEvent(dpp::ready_t const& event);
        void HandleSlashCommand(dpp::slashcommand_t const& event);
        void HandleFormSubmitEvent(dpp::form_submit_t const& event);
        void HandleLogEvent(dpp::log_t const& event);
        void HandleSelectClickEvent(dpp::select_click_t const& event);

    public: // Command handlers
        void OnListProductCommand(dpp::slashcommand_t const& event, std::string const& product);
        void OnDownloadCommand(dpp::slashcommand_t const& event, std::string const& product, std::string const& file);

    private:
        backend::Database _database;
        tact::data::product::Manager& _productManager;

        dpp::cluster _bot;
        dpp::commandhandler _handler;

        std::shared_ptr<spdlog::logger> _logger;
    };
}

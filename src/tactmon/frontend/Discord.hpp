#pragma once

#include "backend/Database.hpp"

#include <memory>
#include <string>
#include <string_view>

#include <dpp/dpp.h>
#include <spdlog/logger.h>

#include <boost/asio/io_context.hpp>
#include <boost/asio/io_context_strand.hpp>
#include <boost/asio/post.hpp>

namespace tact::data::product {
    struct Manager;
}

namespace frontend {
    struct Discord {
        Discord(boost::asio::io_context::strand strand, std::string_view token,
            tact::data::product::Manager& manager, backend::Database& database);

        void Run();

    private:
        void HandleReadyEvent(dpp::ready_t const& event);
        void HandleSlashCommand(dpp::slashcommand_t const& event);
        void HandleFormSubmitEvent(dpp::form_submit_t const& event);
        void HandleLogEvent(dpp::log_t const& event);
        void HandleSelectClickEvent(dpp::select_click_t const& event);
        void HandleAutoCompleteEvent(dpp::autocomplete_t const& event);

    public: // Command handlers
        void OnListProductCommand(dpp::slashcommand_t const& event, std::string const& product);
        void OnDownloadCommand(dpp::slashcommand_t const& event, std::string const& product, std::string const& version, std::string const& file);

        template <typename T>
        void RunAsync(T&& value) {
            boost::asio::post(_strand, value);
        }

    private:
        tact::data::product::Manager& _productManager;
        backend::Database& _database;

        boost::asio::io_context::strand _strand;
        std::shared_ptr<spdlog::logger> _logger;

        dpp::cluster _bot;
    };
}

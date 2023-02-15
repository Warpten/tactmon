#pragma once

#include "frontend/Commands/ICommand.hpp"

namespace frontend::commands {
    struct DownloadCommand final : ICommand {
        explicit DownloadCommand() { }

        bool Matches(dpp::interaction const& evnt) const override;

        dpp::slashcommand GetRegistrationInfo(dpp::cluster& bot) const override;

        void HandleSlashCommand(dpp::slashcommand_t const& evnt, frontend::Discord& cluster) override;
        void HandleAutoCompleteEvent(dpp::autocomplete_t const& evnt, frontend::Discord& cluster) override;

    private:
        dpp::snowflake _messageID;
    };
}

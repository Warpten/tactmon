#pragma once

#include "frontend/commands/ICommand.hpp"

namespace frontend::commands {
    struct BindCommand final : ICommand {
        explicit BindCommand() { }

        bool Matches(dpp::interaction const& evnt) const override;

        dpp::slashcommand GetRegistrationInfo(dpp::cluster& bot) const override;

        void HandleSlashCommand(dpp::slashcommand_t const& evnt, frontend::Discord& cluster) override;
    };
}

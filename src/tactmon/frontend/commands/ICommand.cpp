#include "frontend/commands/ICommand.hpp"
#include "frontend/Discord.hpp"

namespace frontend::commands {
    bool ICommand::OnSlashCommand(dpp::slashcommand_t const& evnt, frontend::Discord& cluster) {
        if (!Matches(evnt.command))
            return false;

        cluster.RunAsync([this, evnt, &cluster]() {
            this->HandleSlashCommand(evnt, cluster);
        });

        return true;
    }

    bool ICommand::OnAutoComplete(dpp::autocomplete_t const& evnt, frontend::Discord& cluster) {
        if (!Matches(evnt.command))
            return false;

        cluster.RunAsync([this, evnt, &cluster]() {
            this->HandleAutoCompleteEvent(evnt, cluster);
        });

        return true;
    }

    bool ICommand::OnSelectClick(dpp::select_click_t const& evnt, frontend::Discord& cluster) {
        if (!Matches(evnt.command))
            return false;

        cluster.RunAsync([this, evnt, &cluster]() {
            this->HandleSelectClickEvent(evnt, cluster);
        });

        return true;
    }
}

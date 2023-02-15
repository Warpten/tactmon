#pragma once

#include "backend/Database.hpp"

#include <dpp/cluster.h>
#include <dpp/dispatcher.h>

#include <memory>

namespace frontend {
    struct Discord;
}

namespace frontend::commands {
    struct ICommand {
        virtual ~ICommand() = default;

        /**
         * Determines if this command can process a given event.
         * 
         * @param[in] evnt The slash command event.
         */
        virtual bool Matches(dpp::interaction const& evnt) const = 0;

        /**
        * Returns command registration information for Discord.
        */
        virtual dpp::slashcommand GetRegistrationInfo(dpp::cluster& bot) const = 0;

        bool OnSlashCommand(dpp::slashcommand_t const& evnt, frontend::Discord& cluster);
        bool OnAutoComplete(dpp::autocomplete_t const& evnt, frontend::Discord& cluster);
        bool OnSelectClick(dpp::select_click_t const& evnt, frontend::Discord& cluster);

    protected:
        /**
         * Handles Discord's autocomplete events.
         * 
         * @param[in] event The event sent by Discord.
         * @returns A boolean indicating wether or not the event was handled.
         */
        virtual void HandleAutoCompleteEvent(dpp::autocomplete_t const& evnt, frontend::Discord& cluster) { }

        /**
         * Main entry point for a slash command. You usually want to persist the instance of this type
         * if this function ends up returning true, and can store state in this object.
         */
        virtual void HandleSlashCommand(dpp::slashcommand_t const& evnt, frontend::Discord& cluster) { }

        /**
         * Handles click event on command select menus.
         */
        virtual void HandleSelectClickEvent(dpp::select_click_t const& evnt, frontend::Discord& cluster) { }
    };
}

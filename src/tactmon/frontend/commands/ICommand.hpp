#pragma once

#include "backend/Database.hpp"

#include <dpp/cluster.h>
#include <dpp/dispatcher.h>

#include <memory>

namespace frontend {
    struct Discord;
}

namespace frontend::commands {
    struct AutocompletionContext final {
        explicit AutocompletionContext(dpp::autocomplete_t const& evnt) : evnt(evnt) { }

        dpp::autocomplete_t const& evnt;
        std::unordered_map<std::string, dpp::command_value> values;

        template <typename T>
        std::optional<T> get(std::string const& name) {
            auto itr = values.find(name);
            if (itr != values.end() && std::holds_alternative<T>(itr->second))
                return std::get<T>(itr->second);

            return std::nullopt;
        }

        bool has(std::string token) {
            return values.find(token) != values.end();
        }
    };

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
        template <typename T>
        static std::optional<T> GetParameter(dpp::slashcommand_t const& evnt, std::string const& name) {
            // auto topLevelValue = evnt.get_parameter<T>(name);
            // if (std::holds_alternative<T>(topLevelValue))
            //     return std::get<T>(topLevelValue);
            // 
            dpp::command_interaction interaction = evnt.command.get_command_interaction();
            for (dpp::command_data_option const& option : interaction.options) {
                std::optional<T> childValue = GetParameter<T>(option, name);
                if (childValue.has_value())
                    return childValue;
            }

            return std::nullopt;
        }

        template <typename T>
        static std::optional<T> GetParameter(dpp::command_data_option const& option, std::string const& name) {
            if (option.name == name) {
                if (std::holds_alternative<T>(option.value))
                    return std::get<T>(option.value);
                return std::nullopt;
            }

            for (dpp::command_data_option const& child : option.options) {
                std::optional<T> childValue = GetParameter<T>(child, name);
                if (childValue.has_value())
                    return childValue;
            }

            return std::nullopt;
        }

        /**
         * Handles Discord's autocomplete events.
         * 
         * @param[in] event The event sent by Discord.
         * @returns A boolean indicating wether or not the event was handled.
         */
        void HandleAutoCompleteEvent(dpp::autocomplete_t const& evnt, frontend::Discord& cluster);

        /**
         * Handles Discord's autocomplete events.
         */
        virtual bool HandleAutoComplete(std::string const& name, dpp::command_value const& value, AutocompletionContext context, frontend::Discord& cluster);

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

#include "frontend/commands/ICommand.hpp"
#include "frontend/Discord.hpp"
#include "utility/Logging.hpp"

namespace db = backend::db;
namespace entity = db::entity;
namespace build = entity::build;
namespace product = entity::product;
namespace tracked_file = entity::tracked_file;

namespace frontend::commands {
    bool ICommand::OnSlashCommand(dpp::slashcommand_t const& evnt, frontend::Discord& cluster) {
        if (!Matches(evnt.command))
            return false;

        cluster.RunAsync([this, evnt, &cluster]() {
            try {
                this->HandleSlashCommand(evnt, cluster);
            } catch (std::exception const& ex) {
                cluster.logger->error(ex.what());
            }
        });

        return true;
    }

    bool ICommand::OnAutoComplete(dpp::autocomplete_t const& evnt, frontend::Discord& cluster) {
        if (!Matches(evnt.command))
            return false;

        cluster.RunAsync([this, evnt, &cluster]() {
            try {
                this->HandleAutoCompleteEvent(evnt, cluster);
            } catch (std::exception const& ex) {
                cluster.logger->error(ex.what());
            }
        });

        return true;
    }

    bool ICommand::OnSelectClick(dpp::select_click_t const& evnt, frontend::Discord& cluster) {
        if (!Matches(evnt.command))
            return false;

        cluster.RunAsync([this, evnt, &cluster]() {
            try {
                this->HandleSelectClickEvent(evnt, cluster);
            } catch (std::exception const& ex) {
                cluster.logger->error(ex.what());
            }
        });

        return true;
    }

    void ICommand::HandleAutoCompleteEvent(dpp::autocomplete_t const& evnt, frontend::Discord& cluster) {
        AutocompletionContext context { evnt };

        bool drillDown = true;

        std::string focusedName;
        dpp::command_value focusedValue { };

        for (const auto& evntOption : evnt.options) {
            if (evntOption.focused) {
                focusedName = evntOption.name;
                focusedValue = evntOption.value;

                drillDown = false;
            }
        }

        dpp::autocomplete_interaction interaction = evnt.command.get_autocomplete_interaction();
        std::vector<dpp::command_data_option> eopts = interaction.options;
        while (drillDown && !eopts.empty()) {
            dpp::command_data_option current = eopts.front();

            for (const auto& eopt : current.options) {
                if (eopt.focused) {
                    focusedName = eopt.name;
                    focusedValue = eopt.value;

                    drillDown = false;
                }
                else {
                    context.values.emplace(eopt.name, eopt.value);
                }
            }

            // We found a focused parameter, stop drilling down
            if (!drillDown)
                break;

            eopts = current.options;
        }

        if (focusedName.empty() || std::holds_alternative<std::monostate>(focusedValue) || !HandleAutoComplete(focusedName, focusedValue, std::move(context), cluster))
            cluster.bot.interaction_response_create_sync(evnt.command.id, evnt.command.token, dpp::interaction_response { dpp::ir_autocomplete_reply });
    }

    bool ICommand::HandleAutoComplete(std::string const& name, dpp::command_value const& value, AutocompletionContext context, frontend::Discord& cluster) {
        if (name == "product") {
            std::string optionValue = std::get<std::string>(value);
            if (optionValue.empty())
                return false;

            size_t suggestionCount = 0;
            dpp::interaction_response interactionResponse{ dpp::ir_autocomplete_reply };
            cluster.db.products.WithValues([&interactionResponse, &suggestionCount, &optionValue](auto entries) {
                for (entity::product::Entity const& entry : entries) {
                    std::string productName = db::get<product::name>(entry);
                    if (productName.find(optionValue) == std::string::npos)
                        continue;

                    ++suggestionCount;
                    interactionResponse.add_autocomplete_choice(dpp::command_option_choice(productName, productName));
                    if (suggestionCount >= 100)
                        break;
                }
            });

            cluster.bot.interaction_response_create_sync(context.evnt.command.id, context.evnt.command.token, interactionResponse);
            return true;
        }

        if (name == "version") {
            std::string optionValue = std::get<std::string>(value);
            if (optionValue.empty())
                return false;

            std::optional<std::string> productFilter = context.get<std::string>("product");
            std::optional<std::string> regionFilter = context.get<std::string>("region");

            dpp::interaction_response interactionResponse{ dpp::ir_autocomplete_reply };
            std::unordered_set<std::string> uniqueBuildNames;
            cluster.db.builds.WithValues([&](auto entries) {
                for (entity::build::Entity const& entry : entries) {
                    std::string buildName = db::get<build::build_name>(entry);
                    if (buildName.find(optionValue) == std::string::npos)
                        continue;

                    if (uniqueBuildNames.count(buildName) > 0)
                        continue;

                    if (productFilter.has_value()) {
                        std::string productName = db::get<build::product_name>(entry);
                        if (productName.find(productFilter.value()) == std::string::npos)
                            continue;
                    }

                    if (regionFilter.has_value()) {
                        std::string regionName = db::get<build::region>(entry);
                        if (regionName.find(regionFilter.value()) == std::string::npos)
                            continue;
                    }

                    uniqueBuildNames.insert(buildName);
                    interactionResponse.add_autocomplete_choice(dpp::command_option_choice(buildName, buildName));
                    // Limit to 100 values
                    if (uniqueBuildNames.size() >= 100)
                        break;
                }
            });

            cluster.bot.interaction_response_create_sync(context.evnt.command.id, context.evnt.command.token, interactionResponse);
            return true;
        }

        if (name == "filepath" && context.has("add")) {
            std::string optionValue = std::get<std::string>(value);
            if (optionValue.empty())
                return false;

            std::optional<std::string> productFilter = context.get<std::string>("product");

            size_t suggestionCount = 0;
            dpp::interaction_response interactionResponse{ dpp::ir_autocomplete_reply };
            cluster.db.trackedFiles.WithValues([&](auto entries) {
                for (tracked_file::Entity const& entry : entries) {
                    if (productFilter.has_value()) {
                        std::string productName = db::get<tracked_file::product_name>(entry);
                        if (productName.find(productFilter.value()) == std::string::npos)
                            continue;
                    }

                    std::string const& displayName = db::get<tracked_file::display_name>(entry);
                    std::string const& filePath = db::get<tracked_file::file_path>(entry);
                    if (filePath.find(optionValue) == std::string::npos)
                        continue;

                    ++suggestionCount;
                    interactionResponse.add_autocomplete_choice(dpp::command_option_choice(displayName, filePath));
                    if (suggestionCount >= 100)
                        break;
                }
            });

            cluster.bot.interaction_response_create_sync(context.evnt.command.id, context.evnt.command.token, interactionResponse);
            return true;
        }

        return false;
    }
}

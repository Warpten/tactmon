#include "backend/db/entity/Build.hpp"
#include "backend/db/repository/Build.hpp"
#include "frontend/Discord.hpp"

#include <ext/Literal.hpp>
#include <logging/Sinks.hpp>
#include <tact/data/product/Manager.hpp>

#include <cstdint>
#include <format>
#include <functional>
#include <string>
#include <string_view>
#include <tuple>
#include <type_traits>

#include <spdlog/spdlog.h>

namespace frontend {
    namespace detail {
        template <typename T> struct is_optional : std::false_type { };
        template <typename T> struct is_optional<std::optional<T>> : std::true_type { };

        template <typename T>
        constexpr static const bool is_optional_v = is_optional<T>::value;

        template <dpp::command_option_type T> struct parameter_type_map { using type = void; };
        template <> struct parameter_type_map<dpp::co_string> {
            using type = std::string;
        };
        template <> struct parameter_type_map<dpp::co_integer> {
            using type = int64_t;
        };
        template <> struct parameter_type_map<dpp::co_number> {
            using type = double;
        };
        template <> struct parameter_type_map<dpp::co_boolean> { 
            using type = bool;
        };

        template <dpp::command_option_type T>
        using dpp_type_t = typename parameter_type_map<T>::type;

        template <typename T> struct remove_optional : std::type_identity<T> { };
        template <typename T> struct remove_optional<std::optional<T>> : std::type_identity<T> { };

        template <typename T>
        using remove_optional_t = typename remove_optional<T>::type;
    }

    template <ext::Literal N, dpp::command_option_type T, ext::Literal D, bool O = false, bool A = false>
    struct Parameter {
        constexpr static const std::string_view Name = N.Value;
        constexpr static const bool IsOptional = O;
        constexpr static const bool IsAutoComplete = A;
        constexpr static const dpp::command_option_type Type = T;
        constexpr static const std::string_view Description = D.Value;

        using value_type = std::conditional_t<O, std::optional<detail::dpp_type_t<T>>, detail::dpp_type_t<T>>;
    };

    template <typename... Args>
    struct Command {
        std::string Name;
        std::string Description;
        std::function<void(Discord*, const dpp::slashcommand_t&, typename Args::value_type&&... args)> Handler;

        void Register(Discord* self, dpp::cluster& bot) {
            dpp::slashcommand command(Name, Description, bot.me.id);
            (Register_<Args>(command), ...);

            bot.global_command_create(command);
        }

        bool TryHandle(Discord* self, dpp::slashcommand_t const& event) {
            if (event.command.get_command_name() != Name)
                return false;

            std::apply(Handler, 
                std::tuple_cat(
                    std::tuple { self, std::cref(event) }, 
                    Parse_(event, std::index_sequence_for<Args...>())
                )
            );
            return true;
        }

    private:
        template <typename P>
        static void Register_(dpp::slashcommand& slashCommand) {
            slashCommand.add_option(dpp::command_option{ P::Type, std::string{ P::Name }, std::string{ P::Description }, !P::IsOptional }
                .set_auto_complete(P::IsAutoComplete));
        }

        template <size_t... Is>
        static auto Parse_(const dpp::slashcommand_t& event, std::index_sequence<Is...>) {
            return std::tuple { Parse_<Args, Is>(event)... };
        }

        template <typename P, size_t I>
        static auto Parse_(const dpp::slashcommand_t& event) -> typename P::value_type {
            using non_optional_t = detail::remove_optional_t<typename P::value_type>;

            const dpp::command_value& value = event.get_parameter(std::string { P::Name });
            if (std::holds_alternative<non_optional_t>(value))
                return std::get<non_optional_t>(value);

            if constexpr (P::IsOptional)
                return std::nullopt;
            else
                return non_optional_t { };
        }
    };

    Command<
        Parameter<"product", dpp::co_string, "The name of the product", false, true>
    > ProductListCommand {
        .Name = "list",
        .Description = "Lists known versions of a specific product.",
        .Handler = &Discord::OnListProductCommand
    };

    Command<
        Parameter<"product", dpp::co_string, "The name of the product", false, true>,
        Parameter<"file", dpp::co_string, "Complete path to the file", false, true>
    > DownloadCommand{
        .Name = "download",
        .Description = "Downloads a file for a specific build",
        .Handler = &Discord::OnDownloadCommand
    };

    namespace db = backend::db;
    namespace entity = db::entity;
    namespace build = entity::build;

    Discord::Discord(std::string_view token, tact::data::product::Manager& productManager, backend::Database&& database)
        : _bot(std::string{ token }), _productManager(productManager), _database(std::move(database))
    {
        _logger = logging::GetLogger("discord");

        _bot.on_ready(std::bind(&Discord::HandleReadyEvent, this, std::placeholders::_1));
        _bot.on_slashcommand(std::bind(&Discord::HandleSlashCommand, this, std::placeholders::_1));
        _bot.on_form_submit(std::bind(&Discord::HandleFormSubmitEvent, this, std::placeholders::_1));
        _bot.on_select_click(std::bind(&Discord::HandleSelectClickEvent, this, std::placeholders::_1));
        _bot.on_log(std::bind(&Discord::HandleLogEvent, this, std::placeholders::_1));
    }

    void Discord::Run() {
        _bot.start(dpp::start_type::st_wait);
    }

    void Discord::HandleLogEvent(dpp::log_t const& event) {
        switch (event.severity) {
            case dpp::ll_trace:   _logger->trace("{}", event.message); break;
            case dpp::ll_debug:   _logger->debug("{}", event.message); break;
            case dpp::ll_info:    _logger->info("{}", event.message); break;
            case dpp::ll_warning: _logger->warn("{}", event.message); break;
            case dpp::ll_error:   _logger->error("{}", event.message); break;
            default:              _logger->critical("{}", event.message); break;
        }
    }

    void Discord::HandleReadyEvent(dpp::ready_t const& event) {
        if (!dpp::run_once<struct command_registration_handler>())
            return;

        DownloadCommand.Register(this, _bot);
        ProductListCommand.Register(this, _bot);
    }

    void Discord::HandleSlashCommand(dpp::slashcommand_t const& event) {
        if (ProductListCommand.TryHandle(this, event)) return;
        if (DownloadCommand.TryHandle(this, event)) return;
    }

    void Discord::HandleAutoCompleteEvent(dpp::autocomplete_t const& event) {

    }

    void Discord::HandleSelectClickEvent(dpp::select_click_t const& event) {
        if (event.command.get_command_name() != "client_build_for_download") {
            std::string productName = std::get<std::string>(event.get_parameter("product"));
            std::string file = std::get<std::string>(event.get_parameter("file"));

            uint32_t selectedBuildID = 0;
            auto validBuildID = [&]() {
                if (!event.values[0].starts_with("key-"))
                    return false;

                auto [ptr, ec] = std::from_chars(event.values[0].data() + 4, event.values[0].data() + event.values[0].size(), selectedBuildID);
                return ec == std::errc { };
            }();

            if (!validBuildID) {
                event.edit_response(dpp::message().add_embed(
                    dpp::embed()
                        .set_color(0x00FF0000u)
                        .set_title(productName)
                        .set_description("Invalid build received")
                ));
            }

            // Load product on the specific build config
            // Do it asynchronously (even though this function call itself is async, we don't want it to starve the thread
            // managing it)
            auto product = _productManager.Rent(productName);
            if (product == nullptr) {
                event.edit_response(dpp::message().add_embed(
                    dpp::embed()
                        .set_color(0x00FF0000u)
                        .set_title(productName)
                        .set_description("This product is not currently tracked.")
                ));

                return;
            }

            auto buildEntry = _database.GetBuildRepository()[selectedBuildID];
            if (!buildEntry.has_value()) {
                event.edit_response(dpp::message().add_embed(
                    dpp::embed()
                        .set_title(productName)
                        .set_color(0x00FF0000u)
                        .set_description("Build configuration cannot be found.")
                        // .set_footer(dpp::embed_footer()
                        //     .set_text(selectedBuildName))
                ));

                return;
            }

            std::string buildName { db::get<build::build_name>(*buildEntry) };
            std::string buildConfig { db::get<build::build_config>(*buildEntry) };
            std::string cdnConfig { db::get<build::cdn_config>(*buildEntry) };

            if (!product->Load(buildConfig, cdnConfig)) {
                event.edit_response(dpp::message().add_embed(
                    dpp::embed()
                        .set_title(productName)
                        .set_color(0x00FF0000u)
                        .set_description("An internal error occured.")
                        .set_footer(dpp::embed_footer()
                            .set_text(std::format("{} - {} / {}", buildName, buildConfig, cdnConfig))
                        )
                    )
                );

                return;
            }

            std::optional<tact::data::FileLocation> fileLocation = product->FindFile(file);
            if (!fileLocation.has_value()) {
                event.edit_response(dpp::message().add_embed(
                    dpp::embed()
                        .set_title(productName)
                        .set_color(0x00FF0000u)
                        .set_description(std::format("`{}` does not exist.", file))
                        .set_footer(dpp::embed_footer()
                            .set_text(buildName))
                ));
                
                return;
            }

            // Generate a link to the http server
            // The server will perform routing over all available CDN servers for the provided build
            // and stream the response to the client.
            event.edit_response(dpp::message().add_embed(
                dpp::embed()
                    .set_title("Download this file.")
                    .set_url("http://www.google.fr") // TODO: Generate link
                    .set_description(std::format("Click here to download `{}`", file))
                    .set_footer(dpp::embed_footer()
                        .set_text(buildName))
            ));
        }
    }

    void Discord::HandleFormSubmitEvent(dpp::form_submit_t const& event) {
    }

    void Discord::OnListProductCommand(dpp::slashcommand_t const& event, std::string const& product) {
        auto entity = _database.GetBuildRepository().GetStatisticsForProduct(product);
        if (!entity.has_value()) {
            event.reply(std::format("No version found for the **{}** product.", product));

            return;
        }

        event.reply(dpp::message().add_embed(
            dpp::embed()
                .set_color(0x0000FF00u)
                .set_description(
                    std::format("Most recent build: **{}**", db::get<build::build_name>(*entity))
                )
                .set_footer(dpp::embed_footer()
                    .set_text(
                        std::format("**{}** known builds for product **{}**.", db::get<build::dto::columns::id_count>(*entity), product)
                    )
                )
        ));
    }

    void Discord::OnDownloadCommand(dpp::slashcommand_t const& event, std::string const& product, std::string const& file) {
        auto eligibleBuilds = _database.GetBuildRepository().GetByProductName(product);
        if (eligibleBuilds.empty()) {
            event.reply(dpp::message().add_embed(
                dpp::embed()
                    .set_color(0x00FF0000u)
                    .set_description("No known builds for this product.")
                    .set_footer(dpp::embed_footer()
                        .set_text(product))
                )
            );

            return;
        }

        dpp::component buildSelectionComponent = dpp::component()
            .set_label("Client build")
            .set_type(dpp::cot_selectmenu)
            .set_id("client_build_for_download")
            .set_placeholder("Please select a build");

        for (build::dto::BuildName const& entity : eligibleBuilds)
            buildSelectionComponent.add_select_option(
                dpp::select_option(
                    db::get<build::build_name>(entity),
                    std::format("key-{}", db::get<build::id>(entity))
                )
            );

        event.reply(dpp::message()
            .set_content(std::format("Select the client build for which you want to download `{}`.", file))
            .add_component(dpp::component()
                .set_type(dpp::cot_action_row)
                .add_component(buildSelectionComponent)
                .add_component(dpp::component().set_default_value(product))
                .add_component(dpp::component().set_default_value(file))
            )
        );
    }
}

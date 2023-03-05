#include "backend/Database.hpp"
#include "backend/Product.hpp"
#include "backend/ProductCache.hpp"
#include "backend/RibbitMonitor.hpp"
#include "frontend/Discord.hpp"
#include "net/Server.hpp"
#include "utility/Logging.hpp"
#include "utility/ThreadPool.hpp"

#include <libtactmon/ribbit/Commands.hpp>
#include <libtactmon/ribbit/types/Versions.hpp>
#include <libtactmon/tact/data/product/wow/Product.hpp>

#include <boost/program_options.hpp>
#include <boost/asio/executor_work_guard.hpp>
#include <boost/asio/io_context.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/signal_set.hpp>
#include <boost/asio/thread_pool.hpp>

#include <spdlog/spdlog.h>
#include <spdlog/logger.h>
#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/sinks/rotating_file_sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>

#include <chrono>
#include <csignal>
#include <filesystem>
#include <memory>
#include <string_view>
#include <thread>
#include <utility>

void Execute(boost::program_options::variables_map vm);

using namespace std::string_view_literals;
namespace fs = std::filesystem;
namespace po = boost::program_options;
namespace asio = boost::asio;

namespace db = backend::db;

int main(int argc, char** argv) {
    po::options_description desc("Options");
    desc.add_options()
        ("help,h", "This help prompt.")

        ("discord-thread-count", po::value<uint16_t>()->default_value(8),    "The amount of threads dedicated to the Discord bot.")
        ("discord-token",        po::value<std::string>()->required(),       "The Discord bot's token.")

        ("db-thread-count",      po::value<uint16_t>()->default_value(1),    "The amount of threads dedicated to the database caching mechanisms.")
        ("db-username",          po::value<std::string>()->required(),       "Username used to connect to the PostgreSQL database.")
        ("db-password",          po::value<std::string>()->required(),       "Password used to connect to the PostgreSQL database.")
        ("db-name",              po::value<std::string>()->required(),       "Name of the database to use.")
        ("db-host",              po::value<std::string>()->required(),       "Host of the PostgreSQL database.")
        ("db-port",              po::value<uint64_t>()->default_value(5432), "Port on which the application should connect to a PostgreSQL database.")

        ("http-port",            po::value<uint16_t>()->required(),          "Port on which the http proxy server is active.")
        ("http-thread-count",    po::value<uint16_t>()->required(),          "The amount of threads dedicated to the HTTP proxy server. "
                                                                             "If zero, the server is disabled.")
        ("http-document-root",   po::value<std::string>()->required(),       "A document root that will be used when generating download links to files.")

        ("thread-count",         po::value<uint16_t>()->default_value(std::thread::hardware_concurrency() * 2), "Amount of general purpose threads.")
        ;

    try {
        po::variables_map vm;
        po::store(po::parse_command_line(argc, argv, desc), vm);
        po::notify(vm);

        Execute(std::move(vm));
    } catch (po::error const& ex) {
        std::cerr << ex.what() << '\n';
        std::cout << desc << '\n';

        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}

void Execute(boost::program_options::variables_map vm) {
    // 1. General application context.
    utility::ThreadPool threadPool { vm["thread-count"].as<uint16_t>() };
    for (size_t i = 0; i < threadPool.size(); ++i)
        threadPool.PostWork([](boost::asio::io_context& service) { service.run(); });

    // 2. Setup interrupts handler, enqueue infinite work.
#if defined(WIN32)
    asio::signal_set signals(threadPool.executor(), SIGINT, SIGTERM, SIGBREAK);
#else
    asio::signal_set signals(threadPool.executor(), SIGINT, SIGTERM);
#endif

    signals.async_wait([&threadPool](boost::system::error_code const& ec, int signum) {
        threadPool.Interrupt();
    });

    // 3. Initialize product manager.
    fs::path cacheRoot = std::filesystem::current_path() / "cache";

    libtactmon::tact::Cache localCache { cacheRoot };
    backend::ProductCache productCache { threadPool.executor() };

    constexpr static const std::string_view WOW_PRODUCTS[] = { "wow", "wow_beta", "wow_classic", "wow_classic_beta", "wow_classic_ptr" };
    for (std::string_view gameProduct : WOW_PRODUCTS) {
        productCache.RegisterFactory(std::string { gameProduct }, [&localCache, gameProduct, &threadPool]() -> backend::Product {
            return backend::Product {
                std::make_shared<libtactmon::tact::data::product::wow::Product>(
                    gameProduct, localCache, threadPool.executor(), utility::logging::GetAsyncLogger(gameProduct)
                )
            };
        });
    }

    // 4. Initialize database.
    backend::Database database {
        vm["db-thread-count"].as<uint16_t>(),
        *utility::logging::GetAsyncLogger("database").get(),
        vm["db-username"].as<std::string>(),
        vm["db-password"].as<std::string>(),
        vm["db-host"].as<std::string>(),
        vm["db-port"].as<uint64_t>(),
        vm["db-name"].as<std::string>()
    };

    // 5. Initialize HTTP proxy.
    std::shared_ptr<net::Server> proxyServer = [&]() -> std::shared_ptr<net::Server> {
        size_t threadCount = vm["http-thread-count"].as<uint16_t>();
        if (threadCount == 0)
            return nullptr;

        std::shared_ptr<net::Server> server = std::make_shared<net::Server>(boost::asio::ip::tcp::endpoint {
            boost::asio::ip::tcp::v4(),
            vm["http-port"].as<uint16_t>()
        }, threadCount, vm["http-document-root"].as<std::string>());

        server->Run();
        return server;
    }();

    // 6. Initialize Ribbit monitor.
    backend::RibbitMonitor monitor { threadPool.executor(), database };

    // 7. Initialize discord frontend
    frontend::Discord bot { vm["discord-thread-count"].as<uint16_t>(), vm["discord-token"].as<std::string>(), productCache, database, proxyServer };
    bot.Run();

    // 8. Create listener for ribbit
    monitor.RegisterListener([&](std::string const& productName, uint64_t sequenceID, backend::RibbitMonitor::ProductState state)
    {
        if (!productCache.IsAwareOf(productName))
            return;

        namespace tact = libtactmon::tact;
        namespace ribbit = libtactmon::ribbit;

        auto versions = ribbit::Versions<>::Execute(threadPool.executor(), nullptr, ribbit::Region::US, productName);
        auto cdns = ribbit::CDNs<>::Execute(threadPool.executor(), nullptr, ribbit::Region::US, productName);
        if (!versions.has_value() || !cdns.has_value())
            return;

        std::vector<dpp::embed> embeds;

        for (ribbit::types::versions::Record const& version : versions->Records) {
            tact::data::product::ResourceResolver resolver(threadPool.executor(), localCache);

            std::optional<tact::config::BuildConfig> buildConfig = resolver.ResolveConfiguration<tact::config::BuildConfig>(*cdns, version.BuildConfig,
                [&](libtactmon::io::FileStream& fstream) {
                    return tact::config::BuildConfig::Parse(fstream);
                });

            if (!buildConfig.has_value())
                continue;

            if (!database.builds.GetByBuildName(buildConfig->BuildName, version.Region).has_value())
                database.builds.Insert(version.Region, productName, buildConfig->BuildName, version.BuildConfig, version.CDNConfig);

            dpp::embed embed;
            embed.set_title(fmt::format("`{}` ({})", buildConfig->BuildName, version.Region));
            embed.add_field("Build Configuration", fmt::format("`{}`", version.BuildConfig), true);
            embed.add_field("CDN Configuration", fmt::format("`{}`", version.CDNConfig), true);
            embed.set_color(0x00FF0000u);

            embeds.emplace_back(std::move(embed));

            ss << fmt::format("{:<8} | {:<25} | {:<34} | {:<34}\n", version.Region, buildConfig->BuildName, version.BuildConfig, version.CDNConfig);
        }
        ss << "```";

        std::string embedBody = ss.str();

        database.boundChannels.WithValues([&](auto entries) {
            for (db::entity::bound_channel::Entity::as_projection const& entity : entries) {
                if (db::get<db::entity::bound_channel::product_name>(entity) != productName)
                    continue;

                uint64_t channelID = db::get<db::entity::bound_channel::channel_id>(entity);

                dpp::message message(dpp::snowflake{ channelID });
                message.set_content("Builds update for product `{0}` has been detected (**{1:%Y-%m-%d}** at **{1:%H:%M:%S}**).", productName, std::chrono::system_clock::now());
                for (auto&& embed : embeds)
                    message.add_embed(embed);

                bot.bot.message_create(message);
            }
        });
    });

    // Start updating ribbit state
    monitor.BeginUpdate();

    // 10. Cleanup
    if (proxyServer != nullptr)
        proxyServer->Stop();
}

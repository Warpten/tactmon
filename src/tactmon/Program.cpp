#include "backend/Database.hpp"
#include "backend/Product.hpp"
#include "frontend/Discord.hpp"
#include "frontend/Proxy.hpp"
#include "net/ribbit/Commands.hpp"
#include "tact/data/product/wow/Product.hpp"
#include "logging/Sinks.hpp"

#include <boost/program_options.hpp>
#include <boost/asio/executor_work_guard.hpp>
#include <boost/asio/io_context.hpp>
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

int main(int argc, char** argv) {
    po::options_description desc("Options");
    desc.add_options()
        ("help,h", "This help prompt.")
        ("discord-token", po::value<std::string>()->required(), "The Discord bot's token.")
        ("database-username", po::value<std::string>()->required(), "Username used to connect to the PostgreSQL database.")
        ("database-password", po::value<std::string>()->required(), "Password used to connect to the PostgreSQL database.")
        ("database-host", po::value<std::string>()->required(), "Host of the PostgreSQL database.")
        ("database-port", po::value<uint64_t>()->default_value(5432), "Port on which the database is listening. Defaults to 5432.")
        ("database-name", po::value<std::string>()->required(), "Name of the database to use.")
        ("http-port", po::value<uint16_t>()->required(), "Port on which the http proxy server is active.")
        ("http-proxy-document-root", po::value<std::string>()->required(), "The base of the URL used to generate download links to files.")
        ("thread-count", po::value<uint32_t>()->default_value(std::thread::hardware_concurrency() * 2), "The amount of threads to use.")
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
    asio::io_context ctx;

    // 2. Setup interrupts handler, enqueue infinite work. Unfortunately, this causes a thread to be hogged down...
    asio::executor_work_guard<asio::io_context::executor_type> guard = asio::make_work_guard(ctx);
#if defined(_WIN32)
    asio::signal_set signals(ctx, SIGINT, SIGTERM, SIGBREAK);
#else
    asio::signal_set signals(ctx, SIGINT, SIGTERM);
#endif
    signals.async_wait([&guard, &ctx](boost::system::error_code const& ec, int signum) {
        guard.reset();
        ctx.stop();
    });

    // 3. Create specific strands.
    asio::io_context::strand databaseStrand { ctx }; // Serializes database cache updates.
    asio::io_context::strand discordStrand  { ctx }; // Single-shot strand for the discord bot instance.
    asio::io_context::strand productStrand  { ctx };

    // 4. Create thread pool, initialize threads.
    size_t threadCount = vm["thread-count"].as<uint32_t>();
    asio::thread_pool threadPool{ threadCount };
    for (size_t i = 0; i < threadCount; ++i)
        asio::post(threadPool, [&ctx]() { 
            ctx.run();
        });

    // 5. Initialize product manager.
    fs::path cacheRoot = std::filesystem::current_path() / "cache";

    backend::ProductCache productCache { productStrand };

    constexpr static const std::string_view WOW_PRODUCTS[] = { "wow", "wow_beta", "wow_classic", "wow_classic_beta", "wow_classic_ptr" };
    for (std::string_view gameProduct : WOW_PRODUCTS) {
        productCache.RegisterFactory(std::string { gameProduct }, [cacheRoot, gameProduct, &ctx]() -> backend::Product {
            return backend::Product { std::make_shared<tact::data::product::wow::Product>(gameProduct, cacheRoot, ctx) };
        });
    }

    // 6. Initialize database.
    backend::Database database{
        databaseStrand,
        vm["database-username"].as<std::string>(),
        vm["database-password"].as<std::string>(),
        vm["database-host"].as<std::string>(),
        vm["database-port"].as<uint64_t>(),
        vm["database-name"].as<std::string>()
    };

    // 7. Initialize HTTP proxy.
    frontend::Proxy proxy { ctx, vm["http-proxy-document-root"].as<std::string>(), vm["http-port"].as<uint16_t>() };

    // 8. Initialize discord frontend
    frontend::Discord bot { discordStrand, vm["discord-token"].as<std::string>(), productCache, database, proxy };
    bot.Run();

    // 9. Wait for interrupts or end.
    threadPool.join();
    ctx.stop();
}

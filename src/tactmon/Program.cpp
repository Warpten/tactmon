#include "backend/Database.hpp"
#include "frontend/Discord.hpp"
#include "net/ribbit/Commands.hpp"
#include "tact/data/product/Manager.hpp"
#include "tact/data/product/wow/Product.hpp"
#include "logging/Sinks.hpp"
#include "ThreadPool.hpp"

#include <boost/program_options.hpp>
#include <boost/asio/io_context.hpp>
#include <boost/asio/signal_set.hpp>

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

void Execute(boost::program_options::variables_map vm, std::shared_ptr<boost::asio::io_context> context);

int main(int argc, char** argv) {
    namespace po = boost::program_options;
    namespace asio = boost::asio;

    po::options_description desc("Options");
    desc.add_options()
        ("help,h", "Help prompt")
        ("discord-token,t", po::value<std::string>()->required(), "The Discord bot's token.")
        ("database-username,u", po::value<std::string>()->required(), "Username used to connect to the PostgreSQL database.")
        ("database-password,w", po::value<std::string>()->required(), "Password used to connect to the PostgreSQL database.")
        ("database-host,o", po::value<std::string>()->required(), "Host of the PostgreSQL database.")
        ("database-port,p", po::value<uint64_t>()->default_value(5432), "Port on which the database is listening. Defaults to 5432.")
        ("database-name,n", po::value<std::string>()->required(), "Name of the database to use")
        ;

    try {
        po::variables_map vm;
        po::store(po::parse_command_line(argc, argv, desc), vm);
        po::notify(vm);

        std::shared_ptr<asio::io_context> context = std::make_shared<asio::io_context>();
        asio::executor_work_guard<asio::io_context::executor_type> guard = asio::make_work_guard(*context);

        asio::signal_set signals(*context, SIGINT, SIGTERM);
#if _WIN32
        signals.add(SIGBREAK);
#endif
        signals.async_wait([&guard](boost::system::error_code const& ec, int signum) {
            guard.reset();
        });

        ThreadPool threadPool{ };
        for (size_t i = 0; i < std::thread::hardware_concurrency(); ++i)
            threadPool.Submit([context]() { context->run(); });

        Execute(std::move(vm), context);

        // Interrupt io_context, join and exit
        threadPool.Join();
    } catch (po::error const& ex) {
        std::cerr << ex.what() << '\n';
        std::cout << desc << '\n';

        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}

using namespace std::string_view_literals;
namespace fs = std::filesystem;

void Execute(boost::program_options::variables_map vm, std::shared_ptr<boost::asio::io_context> context) {
    tact::data::product::Manager manager{ *context };
    // Register known products for simplicity
    manager.Register("wow", [](boost::asio::io_context& context) -> std::shared_ptr<tact::data::product::Product> {
        return std::make_shared<tact::data::product::wow::Product>("wow", std::filesystem::current_path() / "cache", context);
    });

    backend::Database database{
        context,
        vm["database-username"].as<std::string>(),
        vm["database-password"].as<std::string>(),
        vm["database-host"].as<std::string>(),
        vm["database-port"].as<uint64_t>(),
        vm["database-name"].as<std::string>()
    };

    boost::asio::post(*context,
        [manager = std::move(manager), database = std::move(database), vm = std::move(vm), &context]() mutable {
            try {
                frontend::Discord bot { vm["discord-token"].as<std::string>(), manager, std::move(database) };
                bot.Run();
            } catch (std::exception const& ex) {
                std::cerr << ex.what() << '\n';
            }
        });
}

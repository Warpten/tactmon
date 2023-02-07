#include "backend/Database.hpp"
#include "frontend/Discord.hpp"
#include "net/ribbit/Commands.hpp"
#include "tact/data/product/Manager.hpp"
#include "tact/data/product/wow/Product.hpp"
#include "logging/Sinks.hpp"

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
#include <string_view>
#include <thread>
#include <utility>

void Execute(boost::program_options::variables_map vm, boost::asio::io_context& context);

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

        asio::io_context ioContext;
        asio::executor_work_guard<asio::io_context::executor_type> guard = asio::make_work_guard(ioContext);

        asio::signal_set signals(ioContext, SIGINT, SIGTERM);
#if _WIN32
        signals.add(SIGBREAK);
#endif
        signals.async_wait([&guard](boost::system::error_code const& ec, int signum) {
            guard.reset();
        });

        std::thread ioContextRunner([](asio::io_context& context) {
            for (;;) {
                try {
                    context.run();

                    // If we get here, run() exited gracefully
                    break;
                }
                catch (...) {
                    // Otherwise we just hit an error; exit faultily.
                }
            }
        }, std::ref(ioContext));

        Execute(std::move(vm), ioContext);

        // Interrupt io_context, join and exit
        guard.reset();
        ioContextRunner.join();
    } catch (po::error const& ex) {
        std::cerr << ex.what() << '\n';
        std::cout << desc << '\n';

        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}

using namespace std::string_view_literals;
namespace fs = std::filesystem;

void Execute(boost::program_options::variables_map vm, boost::asio::io_context& context) {
    tact::data::product::Manager manager{ context };
    // Register known products for simplicity
    manager.Register("wow", [](boost::asio::io_context& context) -> std::shared_ptr<tact::data::product::Product> {
        return std::make_shared<tact::data::product::wow::Product>("wow", std::filesystem::current_path() / "cache", context);
    });

    backend::Database database{
        vm["database-username"].as<std::string>(),
        vm["database-password"].as<std::string>(),
        vm["database-host"].as<std::string>(),
        vm["database-port"].as<uint64_t>(),
        vm["database-name"].as<std::string>()
    };

    std::thread thread([manager = std::move(manager), database = std::move(database), vm = std::move(vm), &context]() mutable {
        try {
            frontend::Discord bot { vm["discord-token"].as<std::string>(), manager, std::move(database) };
            bot.Run();
        }
        catch (std::exception const& ex) {
            std::cerr << ex.what() << '\n';
        }
    });
    thread.detach(); // TODO: Use a notification variable instead, or wire this to the asio context somehow.

#if 0
    tact::data::product::wow::Product wow("wow", std::filesystem::current_path() / "cache", context);
    wow.Refresh();

    std::optional<tact::data::FileLocation> fileLocation = wow.FindFile("sound/music/zonemusic/terokkar/tf_bonewalkuni03.mp3"); // or wow.FindFile(some_fdid)
    if (fileLocation.has_value()) {
        std::optional<tact::BLTE> fileStream = wow.Open(*fileLocation);
    }
#endif
}

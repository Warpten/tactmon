#include "net/ribbit/Commands.hpp"
#include "io/LocalCache.hpp"

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

void Execute(boost::asio::io_context& context);

int main(int argc, char** argv) {
    namespace po = boost::program_options;
    namespace asio = boost::asio;

    po::options_description desc("Options");
    desc.add_options()
        ("help,h", "Help prompt")
        ;

    // Initialize logging
    auto consoleSink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
    consoleSink->set_level(spdlog::level::info);

    // File sink, 100MB max, 3 files rotated
    auto fileSink = std::make_shared<spdlog::sinks::rotating_file_sink_mt>("logs/console.log", 100 * 1024 * 1024, 3);
    fileSink->set_level(spdlog::level::info);

    std::shared_ptr<spdlog::logger> logger = std::make_shared<spdlog::logger>("main", spdlog::sinks_init_list { consoleSink, fileSink });
    logger->set_level(spdlog::level::trace);
    
    spdlog::set_default_logger(logger);

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

        Execute(ioContext);

        // Interrupt io_context, join and exit
        guard.reset();
        ioContextRunner.join();
    } catch (po::error const& ex) {
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}

using namespace std::string_view_literals;
namespace fs = std::filesystem;

void Execute(boost::asio::io_context& context) {
    namespace ribbit = net::ribbit;

    ribbit::CommandExecutor<ribbit::Command::ProductVersions, ribbit::Region::EU, ribbit::Version::V1> versionsCommand { context };
    std::optional<ribbit::types::Versions> versions = versionsCommand("wow"sv); // Call op
    if (!versions.has_value())
        return;

    ribbit::CommandExecutor<ribbit::Command::ProductCDNs, ribbit::Region::EU, ribbit::Version::V1> cdnsCommand{ context };
    std::optional<ribbit::types::CDNs> cdns = cdnsCommand("wow"sv);
    if (!cdns.has_value())
        return;

    io::LocalCache localCache { fs::current_path(), *cdns, (*versions)[0], context };

    using namespace std::chrono_literals;
}
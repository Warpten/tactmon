#include "net/ribbit/Commands.hpp"
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

void Execute(boost::asio::io_context& context);

int main(int argc, char** argv) {
    namespace po = boost::program_options;
    namespace asio = boost::asio;

    po::options_description desc("Options");
    desc.add_options()
        ("help,h", "Help prompt")
        ;

    // Initialize logging
    std::shared_ptr<spdlog::logger> logger = logging::GetLogger("main");
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
    tact::data::product::wow::Product wow("wow", context);
    wow.Refresh();

    using namespace std::chrono_literals;
}

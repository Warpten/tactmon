#include "utility/Logging.hpp"

#include <cstdint>
#include <memory>

#include <spdlog/async.h>
#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/sinks/rotating_file_sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>

namespace utility::logging {
    struct transparent_hash {
        using hash_type = std::hash<std::string_view>;
        using is_transparent = void;

        std::size_t operator()(const char* str) const { return hash_type{}(str); }
        std::size_t operator()(std::string_view str) const { return hash_type{}(str); }
        std::size_t operator()(std::string const& str) const { return hash_type{}(str); }
    };

    struct ManagerBase {
        ManagerBase() {
            static std::once_flag mtx;
            std::call_once(mtx, []() {
                _consoleSink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
                _consoleSink->set_level(spdlog::level::info);

                // File sink, 100MB max, 3 files rotated
                _fileSink = std::make_shared<spdlog::sinks::rotating_file_sink_mt>("logs/console.log", 100 * 1024 * 1024, 3);
                _fileSink->set_level(spdlog::level::info);

                _debugSink = std::make_shared<spdlog::sinks::basic_file_sink_mt>("logs/debug.txt", true);
                _debugSink->set_level(spdlog::level::debug);
            });
        }

    protected:
        static std::shared_ptr<spdlog::sinks::stdout_color_sink_mt> _consoleSink;
        static std::shared_ptr<spdlog::sinks::basic_file_sink_mt> _debugSink;
        static std::shared_ptr<spdlog::sinks::rotating_file_sink_mt> _fileSink;
    };

    std::shared_ptr<spdlog::sinks::stdout_color_sink_mt> ManagerBase::_consoleSink;
    std::shared_ptr<spdlog::sinks::basic_file_sink_mt> ManagerBase::_debugSink;
    std::shared_ptr<spdlog::sinks::rotating_file_sink_mt> ManagerBase::_fileSink;

    template <typename T>
    struct Manager final : private ManagerBase {
        explicit Manager(std::function<std::shared_ptr<T>(std::string, spdlog::sinks_init_list)> factory) : ManagerBase(), _factory(factory) { 
            if constexpr (!std::is_same_v<T, spdlog::async_logger>)
                _consoleSink->set_pattern("[%Y-%d-%m %H:%M:%S %z] [%n] [%^%l%$] [thread %t] %v");
            else
                _consoleSink->set_pattern("[%Y-%d-%m %H:%M:%S %z] [%n] [%^%l%$] [thread %t] %v");
        }

        std::shared_ptr<T> operator [] (std::string_view name) {
            auto itr = _loggers.find(name);
            if (itr != _loggers.end())
                return itr->second;

            std::string key { name };
            auto value = _factory(std::string { name }, spdlog::sinks_init_list{ ManagerBase::_consoleSink, ManagerBase::_fileSink, ManagerBase::_debugSink });

            auto [_, success] = _loggers.try_emplace(key, value);
            if (success)
                return value;

            return nullptr;
        }

        std::function<std::shared_ptr<T>(std::string, spdlog::sinks_init_list)> _factory;
        std::unordered_map<std::string, std::shared_ptr<T>, transparent_hash, std::equal_to<>> _loggers;
    };

    std::shared_ptr<spdlog::logger> GetLogger(std::string_view name) {
        static Manager<spdlog::logger> manager {
            [](std::string name, spdlog::sinks_init_list sinks) {
                return std::make_shared<spdlog::logger>(name, sinks);
            }
        };

        return manager[name];
    }

    std::shared_ptr<spdlog::async_logger> GetAsyncLogger(std::string_view name) {
        static std::once_flag mtx;
        std::call_once(mtx, []() {
            spdlog::init_thread_pool(8192, 2);
        });

        static Manager<spdlog::async_logger> manager{
            [](std::string name, spdlog::sinks_init_list sinks) {
                return std::make_shared<spdlog::async_logger>(name, sinks.begin(), sinks.end(), spdlog::thread_pool(), spdlog::async_overflow_policy::block);
            }
        };

        return manager[name];
    }
}

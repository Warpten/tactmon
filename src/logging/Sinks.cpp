#include "Sinks.hpp"

#include <cstdint>
#include <memory>

#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/sinks/rotating_file_sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>

namespace logging {
	struct transparent_hash {
		using hash_type = std::hash<std::string_view>;
		using is_transparent = void;

		std::size_t operator()(const char* str) const { return hash_type{}(str); }
		std::size_t operator()(std::string_view str) const { return hash_type{}(str); }
		std::size_t operator()(std::string const& str) const { return hash_type{}(str); }
	};

	struct Manager final {
		explicit Manager() {
			_consoleSink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
			_consoleSink->set_level(spdlog::level::info);

			// File sink, 100MB max, 3 files rotated
			_fileSink = std::make_shared<spdlog::sinks::rotating_file_sink_mt>("logs/console.log", 100 * 1024 * 1024, 3);
			_fileSink->set_level(spdlog::level::info);

			_debugSink = std::make_shared<spdlog::sinks::basic_file_sink_mt>("logs/debug.txt", true);
			_debugSink->set_level(spdlog::level::debug);
		}

		std::shared_ptr<spdlog::logger> operator [] (std::string_view name) {
			auto itr = _loggers.find(name);
			if (itr != _loggers.end())
				return itr->second;

			std::string key { name };
			auto value = std::make_shared<spdlog::logger>(key, spdlog::sinks_init_list{ _consoleSink, _fileSink, _debugSink });

			auto [_, success] = _loggers.try_emplace(key, value);
			if (success)
				return value;

			return nullptr;
		}

		std::shared_ptr<spdlog::sinks::stdout_color_sink_mt> _consoleSink;
		std::shared_ptr<spdlog::sinks::basic_file_sink_mt> _debugSink;
		std::shared_ptr<spdlog::sinks::rotating_file_sink_mt> _fileSink;

		std::unordered_map<std::string, std::shared_ptr<spdlog::logger>, transparent_hash, std::equal_to<>> _loggers;
	};

	std::shared_ptr<spdlog::logger> GetLogger(std::string_view name) {
		static Manager manager { };

		return manager[name];
	}
}
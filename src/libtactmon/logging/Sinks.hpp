#pragma once

#include <spdlog/logger.h>
#include <spdlog/async_logger.h>

#include <memory>
#include <string_view>

namespace logging {
    std::shared_ptr<spdlog::logger> GetLogger(std::string_view name);
    std::shared_ptr<spdlog::async_logger> GetAsyncLogger(std::string_view name);
}

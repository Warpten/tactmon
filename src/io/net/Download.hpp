#pragma once

#include <boost/asio/io_context.hpp>

#include <filesystem>
#include <string_view>

namespace io {
    bool Download(boost::asio::io_context& ctx, std::string_view host, std::string_view query, std::filesystem::path filePath);
}
#pragma once

#include "libtactmon/io/FileStream.hpp"
#include "libtactmon/io/MemoryStream.hpp"
#include "libtactmon/tact/Cache.hpp"

#include <cstdint>
#include <filesystem>
#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <string_view>

#include <boost/asio/io_context.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/beast/core/tcp_stream.hpp>
#include <boost/beast/core/flat_buffer.hpp>
#include <boost/beast/http.hpp>

#include <fmt/format.h>

#include <spdlog/logger.h>

namespace libtactmon::net {
    template <typename Body, typename R>
    struct DownloadTask {
        using BodyType = Body;
        using ValueType = typename Body::value_type;

        using ResultType = std::optional<R>;

        using MessageType = boost::beast::http::message<false, Body>;

        explicit DownloadTask(std::string_view resourcePath) : _resourcePath(resourcePath) { }
        DownloadTask(std::string_view resourcePath, size_t offset, size_t length) : DownloadTask(resourcePath) {
            _offset = offset;
            _size = length;
        }

        virtual boost::system::error_code Initialize(ValueType& body) = 0;
        virtual std::optional<R> TransformMessage(MessageType& body) = 0;

        std::optional<R> Run(boost::asio::any_io_executor executor,
            std::string_view host,
            std::shared_ptr<spdlog::logger> logger)
        {
            namespace asio = boost::asio;
            namespace ip = asio::ip;
            using tcp = ip::tcp;

            namespace beast = boost::beast;
            namespace http = boost::beast::http;

            beast::error_code ec;

            if (logger != nullptr)
                logger->trace("Downloading '{}' from {}.", _resourcePath, host);

            beast::tcp_stream stream { executor };
            tcp::resolver r { executor };

            stream.connect(r.resolve(host, "80"), ec);
            if (ec.failed()) {
                if (logger != nullptr)
                    logger->error("An error occured while downloading {} from {}: {}.", _resourcePath, host, ec.message());

                return std::nullopt;
            }

            http::request<http::string_body> req { http::verb::get, _resourcePath, 11 };
            req.set(http::field::host, host);
            if (_offset != 0 && _size != 0)
                req.set(http::field::range, fmt::format("{}-{}", _offset, _offset + _size - 1));

            http::write(stream, req, ec);
            if (ec.failed()) {
                if (logger != nullptr)
                    logger->error("An error occured while downloading {} from {}: {}.", _resourcePath, host, ec.message());

                return std::nullopt;
            }

            http::response_parser<Body> res;
            res.body_limit({ });

            ec = Initialize(res.get().body());
            if (ec.failed()) {
                if (logger != nullptr)
                    logger->error("An error occured while downloading {} from {}: {}.", _resourcePath, host, ec.message());

                return std::nullopt;
            }

            beast::flat_buffer buffer;
            http::read(stream, buffer, res, ec);
            if (ec.failed() || res.get().result() != http::status::ok) {
                if (logger != nullptr)
                    logger->error("An error occured while downloading {} from {}: {}.", _resourcePath, host, ec.message());
            }

            if (res.get().result() == http::status::ok) {
                if (logger != nullptr)
                    logger->trace("Downloaded '{}' from {} ({} bytes)", host, _resourcePath, res.get()[http::field::content_length]);
            }

            stream.socket().shutdown(boost::asio::ip::tcp::socket::shutdown_both);
            return TransformMessage(res.get());
        }

    protected:
        std::string _resourcePath;
        size_t _size = 0;
        size_t _offset = 0;
    };
}

#pragma once

#include "io/fs/FileStream.hpp"
#include "io/mem/MemoryStream.hpp"
#include "tact/Cache.hpp"

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

#include <spdlog/logger.h>

namespace net {
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

        std::optional<R> Run(boost::asio::io_context& context,
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

            beast::tcp_stream stream { context };
            tcp::resolver r { context };

            stream.connect(r.resolve(host, "80"), ec);
            if (ec.failed()) {
                if (logger != nullptr)
                    logger->error("An error occured while downloading {} from {}: {}.", _resourcePath, host, ec.message());

                return std::nullopt;
            }

            http::request<http::string_body> req { http::verb::get, _resourcePath, 11 };
            req.set(http::field::host, host);
            if (_offset != 0 && _size != 0)
                req.set(http::field::range, std::format("{}-{}", _offset, _offset + _size - 1));

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

    struct FileDownloadTask final : DownloadTask<boost::beast::http::file_body, io::FileStream> {
        FileDownloadTask(std::string_view resourcePath, tact::Cache& localCache) noexcept
            : DownloadTask(resourcePath), _localCache(localCache)
        { }

        FileDownloadTask(std::string_view resourcePath, size_t offset, size_t length, tact::Cache& localCache) noexcept
            : DownloadTask(resourcePath, offset, length), _localCache(localCache)
        { }

        boost::system::error_code Initialize(ValueType& body) override;
        std::optional<io::FileStream> TransformMessage(MessageType& body) override;

    private:
        tact::Cache& _localCache;
    };

    struct MemoryDownloadTask : DownloadTask<boost::beast::http::dynamic_body, io::mem::GrowableMemoryStream> {
        using DownloadTask::DownloadTask;

        boost::system::error_code Initialize(ValueType& body) override;
        std::optional<io::mem::GrowableMemoryStream> TransformMessage(MessageType& body) override;
    };
}

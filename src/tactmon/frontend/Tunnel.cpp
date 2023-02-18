#include "frontend/Tunnel.hpp"

#include <tact/config/CDNConfig.hpp>

#include <ext/Tokenizer.hpp>

#include <chrono>

#include <boost/algorithm/hex.hpp>
#include <boost/algorithm/string/case_conv.hpp>
#include <boost/beast/http/read.hpp>
#include <boost/beast/http/write.hpp>
#include <boost/beast/core/ostream.hpp>

#include <fmt/chrono.h>
#include <fmt/format.h>

namespace asio = boost::asio;
namespace ip = asio::ip;
using tcp = ip::tcp;
namespace http = boost::beast::http;

using namespace std::chrono_literals;

namespace db = backend::db;
namespace build = db::entity::build;

namespace frontend {
    Tunnel::Tunnel(boost::asio::io_context& context, std::string_view localRoot, uint16_t listenPort)
        : _context(context), _localRoot(localRoot), _acceptor(context, ip::tcp::endpoint { ip::tcp::v4(), listenPort })
    {
        Accept();
    }

    std::string Tunnel::GenerateAdress(build::Entity const& buildInfo, std::span<const uint8_t> location, std::string_view fileName, size_t decompressedSize) const {
        std::string hexstr;
        boost::algorithm::hex(location.data(), location.data() + location.size(), std::back_inserter(hexstr));
        boost::algorithm::to_lower(hexstr);

        return fmt::format("{}/{}/{}/{}/{}/{}", _localRoot,
            db::get<build::cdn_config>(buildInfo),
            hexstr, 0, 0, decompressedSize,
            fileName);
    }

    std::string Tunnel::GenerateAdress(build::Entity const& buildInfo, tact::data::ArchiveFileLocation const& location, std::string_view fileName, size_t decompressedSize) const {
        return fmt::format("{}/{}/{}/{}/{}/{}/{}", _localRoot,
            db::get<build::cdn_config>(buildInfo),
            location.name(), location.offset(), location.fileSize(), decompressedSize,
            fileName);
    }

    void Tunnel::ProcessRequest(boost::beast::http::request<boost::beast::http::dynamic_body> const& request, boost::beast::http::response<boost::beast::http::dynamic_body>& response) const {
        std::string_view target { request.target() };
        std::vector<std::string_view> tokens = ext::Tokenize(target, '/');

        std::string_view cdnConfig = tokens[0];
        std::string_view archiveName = tokens[1];

        uint64_t offset = 0; {
            auto [ptr, ec] = std::from_chars(tokens[2].data(), tokens[2].data() + tokens[2].size(), offset);
            if (ec != std::errc{ }) {
                response.result(http::status::bad_request);
                response.set(http::field::content_type, "text/plain");

                boost::beast::ostream(response.body()) << "Invalid range start value";
                return;
            }
        }

        uint64_t length = 0; {
            auto [ptr, ec] = std::from_chars(tokens[3].data(), tokens[3].data() + tokens[3].size(), length);
            if (ec != std::errc{ }) {
                response.result(http::status::bad_request);
                response.set(http::field::content_type, "text/plain");

                boost::beast::ostream(response.body()) << "Invalid range length value";
                return;
            }
        }

        uint64_t decompressedSize = 0; {
            auto [ptr, ec] = std::from_chars(tokens[4].data(), tokens[4].data() + tokens[4].size(), decompressedSize);
            if (ec != std::errc{ } || decompressedSize == 0) {
                response.result(http::status::bad_request);
                response.set(http::field::content_type, "text/plain");

                boost::beast::ostream(response.body()) << "Invalid length";
                return;
            }
        }

        std::string_view fileName = tokens[5];

        response.set("X-Tunnel-CDN-Config", cdnConfig);
        response.set("X-Tunnel-Archive-Name", archiveName);
        response.set("X-Tunnel-File-Name", fileName);

        // response.result(http::status::ok);
        // response.set(http::field::content_type, "application/octet-stream");

        // TODO: Parse CDN config given its name, reead it (it should have been cached, it's a hard error if it hasn't)
        //       Then open TCP socket to the first CDN that answers.
        //       Parse data as it comes, as soon as a socket drops, failover to the next CDN and specify a content range to resume download.
        //       If the last CDN fails, hard error (close the socket)
        //       Stream content from the CDN, decoded, to the client.
    }

    void Tunnel::Accept() {
        boost::asio::ip::tcp::socket& socket = _socket.emplace(_context);

        _acceptor.async_accept(*_socket, [&](boost::system::error_code ec) {
            std::make_shared<Connection>(std::move(socket), this)->Run();

            this->Accept();
        });
    }

    Tunnel::Connection::Connection(boost::asio::ip::tcp::socket socket, Tunnel* proxy)
        : _proxy(proxy), _socket(std::move(socket)), _readStrand(proxy->_context), _writeStrand(proxy->_context), _deadline(_socket.get_executor(), 60s)
    { }

    void Tunnel::Connection::Run() {
        AsyncReadRequest();
        _deadline.async_wait([this](boost::system::error_code ec) {
            if (!ec)
                this->_socket.close(ec);
        });
    }

    void Tunnel::Connection::AsyncReadRequest() {
        auto self = shared_from_this();
        http::async_read(_socket, _buffer, _request, boost::asio::bind_executor(_readStrand, [self](boost::beast::error_code ec, size_t bytesTransferred) {
            boost::ignore_unused(bytesTransferred);
            if (!ec)
                self->ProcessRequest();
        }));
    }

    void Tunnel::Connection::ProcessRequest() {
        _response.version(_request.version());
        _response.keep_alive(false);

        switch (_request.method()) {
            case http::verb::get:
                _proxy->ProcessRequest(_request, _response);
                break;
            default:
                _response.result(http::status::bad_request);
                _response.set(http::field::content_type, "text/plain");
                boost::beast::ostream(_response.body()) << "Invalid request-method " << _request.method_string();
                break;
        }

        AsyncWriteResponse();
    }

    void Tunnel::Connection::AsyncWriteResponse() {
        auto self = shared_from_this();

        _response.content_length(_response.body().size());
        http::async_write(_socket, _response, boost::asio::bind_executor(_writeStrand, [self](boost::beast::error_code ec, size_t bytesTransferred) {
            self->_socket.shutdown(tcp::socket::shutdown_send, ec);
            self->_deadline.cancel();
        }));
    }
}

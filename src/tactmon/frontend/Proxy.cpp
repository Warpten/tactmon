#include "frontend/Proxy.hpp"

#include <chrono>

#include <boost/algorithm/hex.hpp>
#include <boost/algorithm/string/case_conv.hpp>
#include <boost/beast/http/read.hpp>
#include <boost/beast/http/write.hpp>
#include <boost/beast/core/ostream.hpp>

namespace asio = boost::asio;
namespace ip = asio::ip;
using tcp = ip::tcp;
namespace http = boost::beast::http;

using namespace std::chrono_literals;

namespace db = backend::db;
namespace build = db::entity::build;

namespace frontend {
    Proxy::Proxy(boost::asio::io_context& context, std::string_view localRoot, uint16_t listenPort)
        : _context(context), _localRoot(localRoot), _acceptor(context, ip::tcp::endpoint { ip::tcp::v4(), listenPort })
    {
        Accept();
    }

    std::string Proxy::GenerateAdress(build::Entity const& buildInfo, std::span<const uint8_t> location, std::string_view fileName) const {
        std::string hexstr;
        boost::algorithm::hex(location.data(), location.data() + location.size(), std::back_inserter(hexstr));
        boost::algorithm::to_lower(hexstr);

        return std::format("{}/{}/{}/{}/{}/{}/{}", _localRoot,
            db::get<build::build_config>(buildInfo),
            db::get<build::cdn_config>(buildInfo),
            hexstr, 0, 0,
            fileName);
    }

    std::string Proxy::GenerateAdress(build::Entity const& buildInfo, tact::data::IndexFileLocation const& location, std::string_view fileName) const {
        return std::format("{}/{}/{}/{}/{}/{}/{}", _localRoot,
            db::get<build::build_config>(buildInfo),
            db::get<build::cdn_config>(buildInfo),
            location.name(), location.offset(), location.fileSize(),
            fileName);
    }

    void Proxy::ProcessRequest(boost::beast::http::request<boost::beast::http::dynamic_body> const& request, boost::beast::http::response<boost::beast::http::dynamic_body>& response) const {

    }

    void Proxy::Accept() {
        boost::asio::ip::tcp::socket& socket = _socket.emplace(_context);

        _acceptor.async_accept(*_socket, [&](boost::system::error_code ec) {
            std::make_shared<Connection>(std::move(socket), this)->Run();

            this->Accept();
        });
    }

    Proxy::Connection::Connection(boost::asio::ip::tcp::socket socket, Proxy* proxy)
        : _socket(std::move(socket)), _readStrand(proxy->_context), _writeStrand(proxy->_context), _deadline(_socket.get_executor(), 60s), _proxy(proxy)
    { }

    void Proxy::Connection::Run() {
        AsyncReadRequest();
        _deadline.async_wait([this](boost::system::error_code ec) {
            if (!ec)
                this->_socket.close(ec);
        });
    }

    void Proxy::Connection::AsyncReadRequest() {
        auto self = shared_from_this();
        http::async_read(_socket, _buffer, _request, boost::asio::bind_executor(_readStrand, [self](boost::beast::error_code ec, size_t bytesTransferred) {
            boost::ignore_unused(bytesTransferred);
            if (!ec)
                self->ProcessRequest();
        }));
    }

    void Proxy::Connection::ProcessRequest() {
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

    void Proxy::Connection::AsyncWriteResponse() {
        auto self = shared_from_this();

        _response.content_length(_response.body().size());
        http::async_write(_socket, _response, boost::asio::bind_executor(_writeStrand, [self](boost::beast::error_code ec, size_t bytesTransferred) {
            self->_socket.shutdown(tcp::socket::shutdown_send, ec);
            self->_deadline.cancel();
        }));
    }
}

#include "frontend/Proxy.hpp"

#include <chrono>

#include <boost/beast/http/read.hpp>
#include <boost/beast/http/write.hpp>
#include <boost/beast/core/ostream.hpp>

namespace asio = boost::asio;
namespace ip = asio::ip;
using tcp = ip::tcp;
namespace http = boost::beast::http;

namespace frontend {
    // NOTE: Order matters here
    Proxy::Proxy(std::shared_ptr<boost::asio::io_context> context, std::string_view localRoot, uint16_t listenPort)
        : _context(context), _localRoot(localRoot),
        _acceptor(*context, ip::tcp::endpoint { ip::tcp::v4(), listenPort })
    {
        ip::tcp::socket socket { *_context };
        _acceptor.async_accept(std::bind(&Proxy::Accept, this, std::placeholders::_1, std::ref(socket)));
    }

    void Proxy::Accept(boost::system::error_code const& ec, boost::asio::ip::tcp::socket& socket) {
        if (!ec)
            std::make_shared<Connection>(std::move(socket));

        _acceptor.async_accept(std::bind(&Proxy::Accept, this, std::placeholders::_1, std::ref(socket)));
    }

    Proxy::Connection::Connection(boost::asio::ip::tcp::socket&& socket)
        : _socket(std::move(socket)), _deadline(_socket.get_executor(), std::chrono::seconds(60))
    {
        ReadRequest();
        CheckDeadline();
    }

    void Proxy::Connection::ReadRequest() {
        auto self = shared_from_this();
        http::async_read(_socket, _buffer, _request, [self](boost::beast::error_code ec, size_t bytesTransferred) {
            boost::ignore_unused(bytesTransferred);
            if (!ec)
                self->ProcessRequest();
        });
    }

    void Proxy::Connection::ProcessRequest() {
        _response.version(_request.version());
        _response.keep_alive(false);

        switch (_request.method()) {
            case http::verb::get:
                // Query blizzard CDNs and stream the data back
                break;
            default:
                _response.result(http::status::bad_request);
                _response.set(http::field::content_type, "text/plain");
                boost::beast::ostream(_response.body()) << "Invalid request-method " << _request.method_string();
                break;
        }

        WriteResponse();
    }

    void Proxy::Connection::WriteResponse() {
        auto self = shared_from_this();

        _response.content_length(_response.body().size());
        http::async_write(_socket, _response, [self](boost::beast::error_code ec, size_t bytesTransferred) {
            self->_socket.shutdown(tcp::socket::shutdown_send, ec);
            self->_deadline.cancel();
        });
    }

    void Proxy::Connection::CheckDeadline() {
        auto self = shared_from_this();

        _deadline.async_wait([self](boost::system::error_code ec) {
            if (!ec)
                self->_socket.close(ec);
        });
    }
}

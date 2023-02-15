#pragma once

#include <memory>
#include <string>
#include <string_view>

#include <boost/asio/io_context.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/steady_timer.hpp>
#include <boost/beast/core/flat_buffer.hpp>
#include <boost/beast/http/dynamic_body.hpp>
#include <boost/beast/http/message.hpp>

namespace frontend {
    struct Proxy {
        explicit Proxy(std::shared_ptr<boost::asio::io_context> context, std::string_view localRoot, uint16_t listenPort);

    private:
        void Accept(boost::system::error_code const& ec, boost::asio::ip::tcp::socket& socket);

        struct Connection : std::enable_shared_from_this<Connection> {
            explicit Connection(boost::asio::ip::tcp::socket&& socket);

            void ReadRequest();
            void ProcessRequest();
            void WriteResponse();
            void CheckDeadline();

            boost::asio::ip::tcp::socket _socket;
            boost::beast::flat_buffer _buffer { 8192 };
            boost::beast::http::request<boost::beast::http::dynamic_body> _request;
            boost::beast::http::response<boost::beast::http::dynamic_body> _response;
            boost::asio::steady_timer _deadline;
        };

        std::shared_ptr<boost::asio::io_context> _context;
        std::string _localRoot;
        boost::asio::ip::tcp::acceptor _acceptor;
    };
}

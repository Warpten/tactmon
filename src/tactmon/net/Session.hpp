#pragma once

#include <cstdint>
#include <memory>

#include <boost/asio/ip/tcp.hpp>
#include <boost/beast/http/dynamic_body.hpp>
#include <boost/beast/http/empty_body.hpp>
#include <boost/beast/http/message_generator.hpp>
#include <boost/beast/http/parser.hpp>
#include <boost/beast/core/tcp_stream.hpp>
#include <boost/beast/core/flat_buffer.hpp>

namespace net {
    struct Session : std::enable_shared_from_this<Session> {
        explicit Session(boost::asio::ip::tcp::socket&& socket) noexcept;

        void Run();

    private:
        void BeginAccept();

        void BeginRead();
        void HandleRead(boost::beast::error_code ec, std::size_t bytesTransferred);

        bool ProcessRequest(boost::beast::http::request_parser<boost::beast::http::empty_body> const& request);

        bool UpdateOutgoing();

        void BeginWrite(boost::beast::http::message_generator&& response);
        void HandleWrite(bool keepAlive, boost::beast::error_code ec, std::size_t bytesTransferred);

    private:
        boost::beast::tcp_stream _stream;
        boost::beast::flat_buffer _buffer;
        std::vector<boost::beast::http::message_generator> _outgoingQueue;

        // Overriden on each subsequent read
        std::optional<boost::beast::http::request_parser<boost::beast::http::empty_body>> _request;
    };
}

#pragma once

#include "backend/db/DSL.hpp"
#include "backend/db/entity/Build.hpp"

#include <memory>
#include <span>
#include <string>
#include <string_view>

#include <tact/data/FileLocation.hpp>

#include <boost/asio/io_context.hpp>
#include <boost/asio/io_context_strand.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/steady_timer.hpp>
#include <boost/beast/core/flat_buffer.hpp>
#include <boost/beast/http/dynamic_body.hpp>
#include <boost/beast/http/message.hpp>

namespace frontend {
    struct Proxy {
        explicit Proxy(boost::asio::io_context& context, std::string_view localRoot, uint16_t listenPort);

        std::string GenerateAdress(backend::db::entity::build::Entity const& buildInfo, tact::data::IndexFileLocation const& location, std::string_view fileName) const;
        std::string GenerateAdress(backend::db::entity::build::Entity const& buildInfo, std::span<const uint8_t> location, std::string_view fileName) const;

    private:
        void Accept();

        void ProcessRequest(boost::beast::http::request<boost::beast::http::dynamic_body> const& request, boost::beast::http::response<boost::beast::http::dynamic_body>& response) const;

        friend struct Connection;
        
        struct Connection : std::enable_shared_from_this<Connection> {
            explicit Connection(boost::asio::ip::tcp::socket socket, Proxy* proxy);

            void Run();

        private:
            void AsyncReadRequest();
            void ProcessRequest();
            void AsyncWriteResponse();

            Proxy* _proxy;
            boost::asio::ip::tcp::socket _socket;
            boost::asio::io_context::strand _readStrand;
            boost::asio::io_context::strand _writeStrand;
            boost::beast::flat_buffer _buffer { 8192 };
            boost::beast::http::request<boost::beast::http::dynamic_body> _request;
            boost::beast::http::response<boost::beast::http::dynamic_body> _response;
            boost::asio::steady_timer _deadline;
        };

        std::optional<boost::asio::ip::tcp::socket> _socket;
        boost::asio::io_context& _context;
        std::string _localRoot;
        boost::asio::ip::tcp::acceptor _acceptor;
    };
}

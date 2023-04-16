#pragma once

#include <algorithm>
#include <cstdlib>
#include <functional>
#include <iostream>
#include <memory>
#include <span>
#include <string>
#include <string_view>
#include <thread>
#include <vector>

#include <boost/beast/core/error.hpp>
#include <boost/asio/executor_work_guard.hpp>
#include <boost/asio/io_context.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/thread_pool.hpp>

#include <libtactmon/tact/data/FileLocation.hpp>

namespace net {
    class Server : public std::enable_shared_from_this<Server> {
        boost::asio::io_context _service;
        boost::asio::ip::tcp::acceptor _acceptor;

        boost::asio::thread_pool _threadPool;
        std::size_t _threadCount;

    public:
        Server(boost::asio::ip::tcp::endpoint endpoint, std::size_t acceptorThreads, std::string const& documentRoot) noexcept;
        void Run();

        void Stop();

        std::string GenerateAdress(std::string_view product, std::span<const uint8_t> location, std::string_view fileName, std::size_t decompressedSize) const;
        std::string GenerateAdress(std::string_view product, libtactmon::tact::data::ArchiveFileLocation const& location, std::string_view fileName, std::size_t decompressedSize) const;

    private:
        void RunThread();

        void BeginAccept();
        void HandleAccept(boost::beast::error_code ec, boost::asio::ip::tcp::socket socket);

        boost::asio::executor_work_guard<boost::asio::io_context::executor_type> _guard;
        std::string _documentRoot;
    };

}

#pragma once

#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/http/message_generator.hpp>
#include <boost/beast/ssl.hpp>
#include <boost/beast/websocket.hpp>
#include <boost/beast/version.hpp>
#include <boost/asio/as_tuple.hpp>
#include <boost/asio/awaitable.hpp>
#include <boost/asio/bind_executor.hpp>
#include <boost/asio/bind_cancellation_slot.hpp>
#include <boost/asio/co_spawn.hpp>
#include <boost/asio/deferred.hpp>
#include <boost/asio/detached.hpp>
#include <boost/asio/dispatch.hpp>
#include <boost/asio/experimental/parallel_group.hpp>
#include <boost/asio/signal_set.hpp>
#include <boost/asio/strand.hpp>
#include <boost/asio/this_coro.hpp>
#include <boost/asio/thread_pool.hpp>
#include <boost/asio/use_awaitable.hpp>

#include <algorithm>
#include <cstdlib>
#include <functional>
#include <iostream>
#include <memory>
#include <string>
#include <thread>
#include <vector>

namespace net {
    class Server : public std::enable_shared_from_this<Server> {
        boost::asio::io_context _service;
        boost::asio::ip::tcp::acceptor _acceptor;

        boost::asio::thread_pool _threadPool;
        std::size_t _threadCount;

    public:
        Server(boost::asio::ip::tcp::endpoint endpoint, size_t acceptorThreads, std::string const& documentRoot) noexcept;
        void Run();

        void Stop();

    private:
        void RunThread();

        void BeginAccept();
        void HandleAccept(boost::beast::error_code ec, boost::asio::ip::tcp::socket socket);

        boost::asio::executor_work_guard<boost::asio::io_context::executor_type> _guard;
        std::string _documentRoot;
    };

}

#include "net/Server.hpp"
#include "net/Session.hpp"

#include <boost/asio/strand.hpp>

// Note: most of the code here is inspired by
//  https://www.boost.org/doc/libs/1_81_0/libs/beast/example/advanced/server-flex-awaitable/advanced_server_flex_awaitable.cpp.
// Why do I need a PhD in fusion energy to understand any of this?
// Why is there no simple guide or documentation (besides technical documentation) on Asio coroutines?

namespace net {
    namespace asio = boost::asio;
    namespace beast = boost::beast;

    Server::Server(boost::asio::io_context& context, asio::ip::tcp::endpoint endpoint, size_t acceptorThreads) noexcept
        : _service(acceptorThreads), _acceptor(boost::asio::make_strand(context)), _threadPool(acceptorThreads), _threadCount(acceptorThreads)
    {
        beast::error_code ec;

        _acceptor.open(endpoint.protocol(), ec);
        if (ec.failed()) return;

        _acceptor.set_option(asio::socket_base::reuse_address { true }, ec);
        if (ec.failed()) return;

        _acceptor.bind(endpoint, ec);
        if (ec.failed()) return;

        _acceptor.listen(asio::socket_base::max_listen_connections, ec);
        if (ec.failed()) return;
    }

    void Server::Run() {
        asio::dispatch(_acceptor.get_executor(), std::bind_front(&Server::BeginAccept, this->shared_from_this()));
    }

    void Server::BeginAccept() {
        // Make a new strand for this connection so that I/O becomes sequential on said strand.
        _acceptor.async_accept(asio::make_strand(_service), std::bind_front(&Server::HandleAccept, this->shared_from_this()));
    }

    void Server::HandleAccept(boost::beast::error_code ec, boost::asio::ip::tcp::socket socket) {
        if (!ec.failed()) {
            // Create an HTTP session and run it. Running it will extend the lifecycle of the session.
            std::make_shared<Session>(std::move(socket))->Run();
        }

        // Listen for another connection.
        BeginAccept();
    }
}

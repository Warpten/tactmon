#include "net/Server.hpp"
#include "net/Session.hpp"

#include <boost/algorithm/hex.hpp>
#include <boost/algorithm/string/case_conv.hpp>
#include <boost/asio/strand.hpp>

#include <fmt/format.h>

// Note: most of the code here is inspired by
//  https://www.boost.org/doc/libs/1_81_0/libs/beast/example/advanced/server-flex-awaitable/advanced_server_flex_awaitable.cpp.
// Why do I need a PhD in fusion energy to understand any of this?
// Why is there no simple guide or documentation (besides technical documentation) on Asio coroutines?

namespace net {
    namespace asio = boost::asio;
    namespace beast = boost::beast;

    Server::Server(asio::ip::tcp::endpoint endpoint, size_t acceptorThreads, std::string const& documentRoot) noexcept
        : _service(acceptorThreads), _acceptor(boost::asio::make_strand(_service)), _threadPool(acceptorThreads), _threadCount(acceptorThreads),
        _guard(boost::asio::make_work_guard(_service)), _documentRoot(documentRoot)
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

    std::string Server::GenerateAdress(std::string_view product, std::span<const uint8_t> location, std::string_view fileName, size_t decompressedSize) const {
        std::string hexstr;
        boost::algorithm::hex(location.data(), location.data() + location.size(), std::back_inserter(hexstr));
        boost::algorithm::to_lower(hexstr);

        return fmt::format("{}/{}/{}/{}/{}/{}", _documentRoot,
            product,
            hexstr, 0, 0, decompressedSize,
            fileName);
    }

    std::string Server::GenerateAdress(std::string_view product, libtactmon::tact::data::ArchiveFileLocation const& location, std::string_view fileName, size_t decompressedSize) const {
        return fmt::format("{}/{}/{}/{}/{}/{}", _documentRoot,
            product,
            location.name(), location.offset(), location.fileSize(), decompressedSize,
            fileName);
    }

    void Server::Run() {
        asio::dispatch(_acceptor.get_executor(), std::bind_front(&Server::BeginAccept, this->shared_from_this()));

        for (size_t i = 0; i < _threadCount; ++i)
            asio::post(_threadPool, std::bind(&Server::RunThread, this->shared_from_this()));
    }

    void Server::Stop() {
        _guard.reset();

        _threadPool.join();
    }

    void Server::RunThread() {
        for (;;) {
            try {
                _service.run();
                break; // Exited cleanly
            } catch (std::exception const& ex) {
                // Exception
            } catch (...) {
                // Well shit
            }
        }
    }

    void Server::BeginAccept() {
        // Make a new strand for this connection so that I/O becomes sequential on said strand.
        _acceptor.async_accept(_service, std::bind_front(&Server::HandleAccept, this->shared_from_this()));
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

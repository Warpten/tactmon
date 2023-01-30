#include "io/net/Download.hpp"
#include "logging/Sinks.hpp"

#include <boost/asio/ip/tcp.hpp>
#include <boost/beast/core/tcp_stream.hpp>
#include <boost/beast/core/flat_buffer.hpp>
#include <boost/beast/http.hpp>

namespace io {
    static auto logger_ = []() {
        auto logger = logging::GetLogger("download");
        logger->set_level(spdlog::level::info);
        return logger;
    }();

    bool Download(boost::asio::io_context& ctx, std::string_view host, std::string_view query, std::filesystem::path filePath) {
        namespace asio = boost::asio;
        namespace ip = asio::ip;
        using tcp = ip::tcp;

        namespace beast = boost::beast;
        namespace http = boost::beast::http;

        beast::error_code ec;

        logger_->trace("Downloading '{}/{}' ...", host, query);

        beast::tcp_stream stream{ ctx };
        tcp::resolver r{ ctx };

        stream.connect(r.resolve(host, "80"), ec);
        if (ec.failed())
            return false;

        http::request<http::string_body> req{ http::verb::get, std::format("/{}", query), 11 };
        req.set(http::field::host, host);

        http::write(stream, req, ec);
        if (ec.failed())
            return false;

        std::filesystem::create_directories(filePath.parent_path());

        http::response_parser<http::file_body> res;
        res.body_limit({ });
        res.get().body().open(filePath.string().data(), boost::beast::file_mode::write, ec);
        if (res.get().result() != boost::beast::http::status::ok || ec.failed())
            return false;

        boost::beast::flat_buffer buffer;
        boost::beast::http::read(stream, buffer, res, ec);
        if (ec.failed())
            return false;

        logger_->trace("Downloaded '{}/{}' to '{}' ({} bytes).", host, query, filePath.string(), res.get()[http::field::content_length]);

        stream.socket().shutdown(boost::asio::ip::tcp::socket::shutdown_both);
        return true;
    }
}

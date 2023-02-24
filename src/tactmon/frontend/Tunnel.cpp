#include "frontend/Tunnel.hpp"
#include "beast/blte_body.hpp"

#include <libtactmon/tact/config/CDNConfig.hpp>
#include <libtactmon/ribbit/Commands.hpp>
#include <libtactmon/ribbit/types/CDNs.hpp>

#include <libtactmon/detail/Tokenizer.hpp>

#include <chrono>
#include <optional>

#include <boost/algorithm/hex.hpp>
#include <boost/algorithm/string/case_conv.hpp>
#include <boost/beast/core/tcp_stream.hpp>
#include <boost/beast/core/flat_buffer.hpp>
#include <boost/beast/core/ostream.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/http.hpp>

#include <fmt/chrono.h>
#include <fmt/format.h>

namespace asio = boost::asio;
namespace ip = asio::ip;
using tcp = ip::tcp;
namespace beast = boost::beast;
namespace http = beast::http;

using namespace std::chrono_literals;

namespace ribbit = libtactmon::ribbit;

namespace frontend {
    Tunnel::Tunnel(libtactmon::tact::Cache& localCache, boost::asio::io_context& context, std::string_view localRoot, uint16_t listenPort)
        : _dataCache(localCache), _context(context), _localRoot(localRoot), _acceptor(context, ip::tcp::endpoint { ip::tcp::v4(), listenPort })
    {
        Accept();
    }

    std::string Tunnel::GenerateAdress(std::string_view product, std::span<const uint8_t> location, std::string_view fileName, size_t decompressedSize) const {
        std::string hexstr;
        boost::algorithm::hex(location.data(), location.data() + location.size(), std::back_inserter(hexstr));
        boost::algorithm::to_lower(hexstr);

        return fmt::format("{}/{}/{}/{}/{}/{}", _localRoot,
            product,
            hexstr, 0, 0, decompressedSize,
            fileName);
    }

    std::string Tunnel::GenerateAdress(std::string_view product, libtactmon::tact::data::ArchiveFileLocation const& location, std::string_view fileName, size_t decompressedSize) const {
        return fmt::format("{}/{}/{}/{}/{}/{}", _localRoot,
            product,
            location.name(), location.offset(), location.fileSize(), decompressedSize,
            fileName);
    }

    struct FileQueryParams {
        std::string Product;
        std::string CDN;
        std::string ArchiveName;
        std::size_t Offset; // In archive
        std::size_t Length; // In archive
        std::size_t DecompressedSize;
        std::string FileName;
    };

    void Tunnel::ProcessRequest(boost::beast::http::request<boost::beast::http::dynamic_body> const& request, boost::beast::http::response<boost::beast::http::dynamic_body>& response) const {
        std::string_view target { request.target() };
        std::vector<std::string_view> tokens = libtactmon::detail::Tokenize(target, '/');
        if (!tokens.empty())
            tokens.erase(tokens.begin());

        auto writeError = [&response](http::status responseCode, std::string_view body) {
            response.result(responseCode);
            response.set(http::field::content_type, "text/plain");

            boost::beast::ostream(response.body()) << body;
        };

        if (tokens.size() != 6)
            return writeError(http::status::not_found, "");

        auto clientStream = boost::beast::ostream(response.body());

        FileQueryParams params;
        params.Product = tokens[0];
        params.ArchiveName = tokens[1];
        
        {
            auto [ptr, ec] = std::from_chars(tokens[2].data(), tokens[2].data() + tokens[2].size(), params.Offset);
            if (ec != std::errc{ })
                return writeError(http::status::bad_request, "Invalid range start value");
        } {
            auto [ptr, ec] = std::from_chars(tokens[3].data(), tokens[3].data() + tokens[3].size(), params.Length);
            if (ec != std::errc{ })
                return writeError(http::status::bad_request, "Invalid range length value");
        } {
            auto [ptr, ec] = std::from_chars(tokens[4].data(), tokens[4].data() + tokens[4].size(), params.DecompressedSize);
            if (ec != std::errc{ } || params.DecompressedSize == 0)
                return writeError(http::status::bad_request, "Invalid length");
        }

        params.FileName = tokens[5];

        response.set("X-Tunnel-Product", params.Product);
        response.set("X-Tunnel-CDN-Config", params.CDN);
        response.set("X-Tunnel-Archive-Name", params.ArchiveName);
        response.set("X-Tunnel-File-Name", params.FileName);

        // 1. Read CDN data from Ribbit
        std::optional<ribbit::types::CDNs> cdns = [](std::string_view product, boost::asio::io_context& ctx) {
            ribbit::CDNs<ribbit::Region::EU> query { ctx };
            return query(nullptr, std::move(product));
        }(params.Product, _context);

        if (!cdns.has_value())
            return writeError(http::status::not_found,
                "Unable to resolve CDN configuration.\r\n"
                "This could be due to Ribbit being unavailable or this build not being cached.\r\n"
                "Try again in a bit; if this error persists, you're out of luck.");

        tcp::resolver resolver{ _context };
        beast::tcp_stream remoteStream{ _context };

        beast::error_code ec;

        // 2. Now that we have a CDN, look for the first available one
        for (ribbit::types::cdns::Record const& cdn : *cdns) {
            std::string_view cdnConfigHash = params.CDN;
            std::string remotePath = std::format("/{}/data/{}/{}/{}", cdn.Path, params.ArchiveName.substr(0, 2), params.ArchiveName.substr(2, 2), params.ArchiveName);

            for (std::string_view host : cdn.Hosts) {
                // Open a HTTP socket to the actual CDN.
                remoteStream.connect(resolver.resolve(host, "80"), ec);
                if (ec.failed())
                    continue;

                http::request<http::dynamic_body> remoteRequest { http::verb::get, remotePath, 11 };
                remoteRequest.set(http::field::host, host);
                if (params.Length != 0)
                    remoteRequest.set(http::field::range, std::format("{}-{}", params.Offset, params.Offset + params.Length - 1));

                http::write(remoteStream, remoteRequest, ec);
                if (ec.failed())
                    continue;

                http::response_parser<boost::beast::user::blte_body> remoteResponse;
                remoteResponse.get().body().open([&clientStream](uint8_t* data, size_t length) {
                    clientStream.write((char*)data, length);
                });

                remoteResponse.body_limit({ });

                beast::flat_buffer remoteBuffer;
                http::read_header(remoteStream, remoteBuffer, remoteResponse, ec);
                if (ec.failed())
                    continue;

                // Not found? Try again on another CDN
                if (remoteResponse.get().result() == http::status::not_found)
                    continue;

                http::read(remoteStream, remoteBuffer, remoteResponse, ec);
                if (!ec.failed())
                    return;
            }
        }
    }

    void Tunnel::Accept() {
        boost::asio::ip::tcp::socket& socket = _socket.emplace(_context);

        _acceptor.async_accept(*_socket, [&](boost::system::error_code ec) {
            std::make_shared<Connection>(std::move(socket), this)->Run();

            this->Accept();
        });
    }

    Tunnel::Connection::Connection(boost::asio::ip::tcp::socket socket, Tunnel* proxy)
        : _proxy(proxy), _socket(std::move(socket)), _readStrand(proxy->_context), _writeStrand(proxy->_context), _deadline(_socket.get_executor(), 60s)
    { }

    void Tunnel::Connection::Run() {
        AsyncReadRequest();
        _deadline.async_wait([this](boost::system::error_code ec) {
            if (!ec)
                this->_socket.close(ec);
        });
    }

    void Tunnel::Connection::AsyncReadRequest() {
        auto self = shared_from_this();
        http::async_read(_socket, _buffer, _request, boost::asio::bind_executor(_readStrand, [self](boost::beast::error_code ec, size_t bytesTransferred) {
            boost::ignore_unused(bytesTransferred);
            if (!ec)
                self->ProcessRequest();
        }));
    }

    void Tunnel::Connection::ProcessRequest() {
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

    void Tunnel::Connection::AsyncWriteResponse() {
        auto self = shared_from_this();

        _response.content_length(_response.body().size());
        http::async_write(_socket, _response, boost::asio::bind_executor(_writeStrand, [self](boost::beast::error_code ec, size_t bytesTransferred) {
            self->_socket.shutdown(tcp::socket::shutdown_send, ec);
            self->_deadline.cancel();
        }));
    }
}

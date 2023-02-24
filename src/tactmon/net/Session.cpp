#include "beast/blte_body.hpp"
#include "net/Session.hpp"

#include <chrono>
#include <string>
#include <string_view>

#include <boost/asio/dispatch.hpp>
#include <boost/beast/http/read.hpp>
#include <boost/beast/http/write.hpp>
#include <boost/beast/core/ostream.hpp>

#include <libtactmon/detail/Tokenizer.hpp>
#include <libtactmon/ribbit/Commands.hpp>
#include <libtactmon/ribbit/types/CDNs.hpp>

using namespace std::chrono_literals;

namespace ribbit = libtactmon::ribbit;
namespace http = boost::beast::http;

namespace net {
    // Maximum number of messages in queue
    constexpr static const size_t QueueLimit = 8;

    Session::Session(boost::asio::ip::tcp::socket&& socket) noexcept : _stream(std::move(socket)) {
        _outgoingQueue.reserve(QueueLimit);
    }

    void Session::Run() {
        boost::asio::dispatch(_stream.get_executor(), std::bind_front(&Session::BeginRead, this->shared_from_this()));
    }

    void Session::BeginRead() {
        _request.emplace();
        _request->body_limit(4092); // Arbitrary limitation to avoid abusers

        _stream.expires_after(60s);

        http::async_read(_stream, _buffer, *_request, std::bind_front(&Session::HandleRead, this->shared_from_this()));
    }

    void Session::HandleRead(boost::beast::error_code ec, std::size_t bytesTransferred) {
        boost::ignore_unused(bytesTransferred);

        if (ec == http::error::end_of_stream) {
            // Client closed the connection, shut down and emit. Our lifecycle will end and our memory will be reclaimed by the OS.
            _stream.socket().shutdown(boost::asio::ip::tcp::socket::shutdown_both, ec);

            return;
        }

        if (ProcessRequest(*_request)) {
            // Begin another read operation if and only if we aren't at the queue limit.
            if (_outgoingQueue.size() < QueueLimit)
                BeginRead();
        }
    }

    struct FileQueryParams {
        std::string Product;
        std::string ArchiveName;
        std::size_t Offset; // In archive
        std::size_t Length; // In archive
        std::size_t DecompressedSize;
        std::string FileName;
    };

    bool Session::ProcessRequest(http::request_parser<http::empty_body> const& request) {
        std::vector<std::string_view> tokens = libtactmon::detail::Tokenize(std::string_view { request.get().target() }, '/');
        if (!tokens.empty())
            tokens.erase(tokens.begin());

        http::response<http::dynamic_body> response;

        auto writeError = [this, &response](http::status responseCode, std::string_view body) {
            response.result(responseCode);
            response.set(http::field::content_type, "text/plain");

            boost::beast::ostream(response.body()) << body;

            this->BeginWrite(http::message_generator { std::move(response) });
            return false;
        };

        if (tokens.size() != 6)
            return writeError(http::status::not_found, "Malformed request");

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

        response.content_length(params.DecompressedSize);
        response.set(http::field::content_disposition, "attachment");
        response.set("X-Tunnel-Product", params.Product);
        response.set("X-Tunnel-Archive-Name", params.ArchiveName);
        response.set("X-Tunnel-File-Name", params.FileName);

        http::response_serializer<http::dynamic_body> responseSerializer { response };

        http::async_write_header(_stream, responseSerializer, 
            [=](boost::system::error_code ec, std::size_t bytesTransferred) {
                boost::ignore_unused(bytesTransferred);
                if (ec.failed())
                    return;
                
                std::optional<ribbit::types::CDNs> cdns = ribbit::CDNs<ribbit::Region::EU>::Execute(_stream.get_executor(), params.Product);
                
                if (!cdns.has_value()) {
                    // TODO: Does this send headers twice? It shouldn't!
                    writeError(http::status::not_found,
                        "Unable to resolve CDN configuration.\r\n"
                        "This could be due to Ribbit being unavailable or this build not being cached.\r\n"
                        "Try again in a bit; if this error persists, you're out of luck.");
                }

                // TODO: This probably needs to happen on a separate executor
                boost::asio::ip::tcp::resolver resolver{ _stream.get_executor() };
                boost::beast::tcp_stream remoteStream{ _stream.get_executor() };

                // 2. Now that we have a CDN, look for the first available one
                for (ribbit::types::cdns::Record const& cdn : *cdns) {
                    std::string remotePath = std::format("/{}/data/{}/{}/{}", cdn.Path, params.ArchiveName.substr(0, 2), params.ArchiveName.substr(2, 2), params.ArchiveName);

                    for (std::string_view host : cdn.Hosts) {
                        // Open a HTTP socket to the actual CDN.
                        remoteStream.connect(resolver.resolve(host, "80"), ec);
                        if (ec.failed())
                            continue;

                        http::request<http::dynamic_body> remoteRequest{ http::verb::get, remotePath, 11 };
                        remoteRequest.set(http::field::host, host);
                        if (params.Length != 0)
                            remoteRequest.set(http::field::range, std::format("{}-{}", params.Offset, params.Offset + params.Length - 1));

                        http::write(remoteStream, remoteRequest, ec);
                        if (ec.failed())
                            continue;

                        http::response_parser<boost::beast::user::blte_body> remoteResponse;
                        remoteResponse.get().body() = [&](std::span<uint8_t const> data) {
                            boost::asio::write(_stream.socket(), boost::asio::buffer(data.data(), data.size()));
                        };

                        remoteResponse.body_limit({ });

                        boost::beast::flat_buffer remoteBuffer;
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
        );
        
        return false;
    }

    void Session::BeginWrite(http::message_generator&& response) {
        _outgoingQueue.emplace_back(std::move(response));

        // If there was no previous work, start the write operation.
        // If there already was work, it'll be picked up in UpdateOutgoing.
        if (_outgoingQueue.size() == 1)
            UpdateOutgoing();
    }

    bool Session::UpdateOutgoing() {
        bool wasFull = _outgoingQueue.size() == QueueLimit;

        if (!_outgoingQueue.empty()) {
            http::message_generator msg = std::move(_outgoingQueue.front());
            _outgoingQueue.erase(_outgoingQueue.begin());

            bool keepAlive = msg.keep_alive();
            boost::beast::async_write(_stream, std::move(msg), std::bind(&Session::HandleWrite, this->shared_from_this(), keepAlive, std::placeholders::_1, std::placeholders::_2));
        }

        return wasFull;
    }

    void Session::HandleWrite(bool keepAlive, boost::beast::error_code ec, std::size_t bytesTransferred) {
        boost::ignore_unused(bytesTransferred);

        if (ec.failed())
            return;

        if (!keepAlive)
            return _stream.socket().shutdown(boost::asio::ip::tcp::socket::shutdown_both, ec);

        if (UpdateOutgoing())
            BeginRead();
    }
}

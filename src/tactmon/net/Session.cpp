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
namespace beast = boost::beast;
namespace http = beast::http;

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

    void Session::HandleRead(beast::error_code ec, std::size_t bytesTransferred) {
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
        boost::system::error_code ec;

        std::vector<std::string_view> tokens = libtactmon::detail::Tokenize(std::string_view { request.get().target() }, '/');
        if (!tokens.empty())
            tokens.erase(tokens.begin());

        http::response<http::dynamic_body> response;

        auto writeError = [this, &response](http::status responseCode, std::string_view body) {
            response.result(responseCode);
            response.set(http::field::content_type, "text/plain");
            response.keep_alive(false);

            beast::ostream(response.body()) << body;

            this->BeginWrite(http::message_generator { std::move(response) });
            return true;
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
        response.keep_alive(false);
        response.set("X-Tunnel-Product", params.Product);
        response.set("X-Tunnel-Archive-Name", params.ArchiveName);
        response.set("X-Tunnel-File-Name", params.FileName);

        std::optional<ribbit::types::CDNs> cdns = ribbit::CDNs<ribbit::Region::EU>::Execute(_stream.get_executor(), params.Product);
        if (!cdns.has_value()) {
            return writeError(http::status::not_found,
                "Unable to resolve CDN configuration.\r\n"
                "This could be due to Ribbit being unavailable or this build not being cached.\r\n"
                "Try again in a bit; if this error persists, you're out of luck.");
        }

        boost::asio::ip::tcp::resolver resolver { _stream.get_executor() };

        // 1. Collect eligible CDNs.
        std::unordered_map<std::string_view, std::string> availableRemoteArchives;
        for (ribbit::types::cdns::Record const& cdn : *cdns) {
            std::string remotePath = std::format("/{}/data/{}/{}/{}", cdn.Path, params.ArchiveName.substr(0, 2), params.ArchiveName.substr(2, 2), params.ArchiveName);

            for (std::string_view host : cdn.Hosts) {
                beast::tcp_stream remoteStream { _stream.get_executor() };
                remoteStream.connect(resolver.resolve(host, "80"), ec);
                if (ec.failed())
                    continue;

                auto checkExistence = [&params, &remotePath, host, &remoteStream](http::verb method) -> bool {
                    beast::error_code ec;

                    http::request<http::dynamic_body> remoteRequest { method, remotePath, 11 };
                    remoteRequest.set(http::field::host, host);
                    if (params.Length != 0)
                        remoteRequest.set(http::field::range, std::format("{}-{}", params.Offset, params.Offset + params.Length - 1));

                    http::write(remoteStream, remoteRequest, ec);
                    if (ec.failed())
                        return false;

                    beast::flat_buffer buffer{ 8192 };
                    http::response_parser<http::dynamic_body> remoteResponse;
                    remoteResponse.body_limit({ });
                    http::read_header(remoteStream, buffer, remoteResponse, ec);
                    if (ec.failed()
                        || remoteResponse.get().result() == http::status::not_found
                        || remoteResponse.get().result() == http::status::range_not_satisfiable
                        || remoteResponse.get().result() == http::status::method_not_allowed)
                        return false;

                    return true;
                };

                if (checkExistence(http::verb::head) || checkExistence(http::verb::get))
                    availableRemoteArchives.emplace(host, remotePath);
            }
        }

        // No CDN can serve that file? Return an error.
        if (availableRemoteArchives.empty())
            return writeError(http::status::not_found, "No CDN could fulfill this request.");

        http::response_serializer<http::dynamic_body> responseSerializer { response };
        http::write_header(_stream, responseSerializer, ec);
        if (ec.failed())
            return true;

        // Note: we need to persist the reader implementation except that it's response_parser::_rd, which is 
        // private and not publicly exposed. Therefore, we store parser state in the value_type itself so we can move it around.
        std::shared_ptr<beast::user::blte_body::value_type_handle> parserState = std::make_shared<beast::user::blte_body::value_type_handle>();
        parserState->handler = [&](std::span<uint8_t const> data) {
            // Update range values for next case if need be
            params.Offset += data.size();
            params.Length -= data.size();

            // Calling Asio write instead of Beast write, because I want the data to send out instantly, not buffer in memory
            boost::asio::write(_stream.socket(), boost::asio::buffer(data.data(), data.size()));
        };

        for (auto [cdn, remotePath] : availableRemoteArchives) {
            beast::tcp_stream remoteStream { _stream.get_executor() };
            remoteStream.connect(resolver.resolve(cdn, "80"), ec);
            if (ec.failed())
                continue;

            http::request<http::dynamic_body> remoteRequest{ http::verb::get, remotePath, 11 };
            remoteRequest.set(http::field::host, cdn);
            if (params.Length != 0)
                remoteRequest.set(http::field::range, std::format("{}-{}", params.Offset, params.Offset + params.Length - 1));

            http::write(remoteStream, remoteRequest, ec);
            if (ec.failed())
                continue;

            http::response_parser<beast::user::blte_body> remoteResponse;
            remoteResponse.body_limit({ });
            remoteResponse.get().body() = parserState;

            // Read header from Blizzard CDN now.
            beast::flat_buffer remoteBuffer;
            http::read_header(remoteStream, remoteBuffer, remoteResponse, ec);
            if (ec.failed())
                continue;

            // Somehow, file went 404? Try another CDN.
            if (remoteResponse.get().result() == http::status::not_found)
                continue;

            // Read the payload
            http::read(remoteStream, remoteBuffer, remoteResponse, ec);

            // Failed? Try again on another CDN.
            if (ec.failed())
                continue;

            // Nothing left to read? Exit.
            if (params.Length == 0)
                return false;
        }

        // If we got here, bail out.
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
            beast::async_write(_stream, std::move(msg), std::bind(&Session::HandleWrite, this->shared_from_this(), keepAlive, std::placeholders::_1, std::placeholders::_2));
        }

        return wasFull;
    }

    void Session::HandleWrite(bool keepAlive, beast::error_code ec, std::size_t bytesTransferred) {
        boost::ignore_unused(bytesTransferred);

        if (ec.failed())
            return;

        if (!keepAlive)
            return _stream.socket().shutdown(boost::asio::ip::tcp::socket::shutdown_both, ec);

        if (UpdateOutgoing())
            BeginRead();
    }
}

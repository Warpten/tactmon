#include "beast/blte_body.hpp"
#include "net/Session.hpp"
#include "utility/ThreadPool.hpp"

#include <chrono>
#include <string>
#include <string_view>

#include <boost/asio/dispatch.hpp>
#include <boost/asio/strand.hpp>
#include <boost/beast/http/read.hpp>
#include <boost/beast/http/write.hpp>
#include <boost/beast/core/ostream.hpp>
#include <boost/thread/future.hpp>

#include <libtactmon/detail/Tokenizer.hpp>
#include <libtactmon/ribbit/Commands.hpp>
#include <libtactmon/ribbit/types/CDNs.hpp>

using namespace std::chrono_literals;

namespace ribbit = libtactmon::ribbit;
namespace beast = boost::beast;
namespace http = beast::http;

namespace net {
    // Maximum number of messages in queue
    constexpr static const std::size_t QueueLimit = 8;

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
        http::response<http::dynamic_body> response;

        auto writeError = [this, &response](http::status responseCode, std::string_view body) {
            response.result(responseCode);
            response.set(http::field::content_type, "text/plain");
            response.keep_alive(false);

            beast::ostream(response.body()) << body;

            this->BeginWrite(http::message_generator{ std::move(response) });
            return true;
        };

        if (request.get().target().find('\\') != std::string::npos)
            return writeError(http::status::bad_request, "Don't try to exploit me");

        std::vector<std::string_view> tokens = libtactmon::detail::CharacterTokenizer<'/'> { std::string_view { request.get().target() }, false }.Accumulate();
        if (!tokens.empty())
            tokens.erase(tokens.begin());

        if (tokens.size() != 6)
            return writeError(http::status::bad_request, "Malformed request");

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

        auto cdns = ribbit::CDNs<>::Execute(_stream.get_executor(), ribbit::Region::US, params.Product);
        if (!cdns.has_value()) {
            return writeError(http::status::not_found,
                "Unable to resolve CDN configuration.\r\n"
                "This could be due to Ribbit being unavailable or this build not being cached.\r\n"
                "Try again in a bit; if this error persists, you're out of luck.");
        }

        // TODO: Probably use a different executor ?
        boost::asio::ip::tcp::resolver resolver { _stream.get_executor() };

        response.content_length(params.DecompressedSize);
        response.set(http::field::content_disposition, "attachment");
        response.keep_alive(false);
        response.set("X-Tunnel-Product", params.Product);
        response.set("X-Tunnel-Archive-Name", params.ArchiveName);
        response.set("X-Tunnel-File-Name", params.FileName);

        http::response_serializer<http::dynamic_body> responseSerializer{ response };
        http::write_header(_stream, responseSerializer, ec);
        if (ec.failed())
            return true;

        // 1. Collect eligible CDNs.
        std::unordered_map<std::string_view, std::string> availableRemoteArchives = CollectAvailableCDNs(*cdns, params.ArchiveName, params.Offset, params.Length);
        if (availableRemoteArchives.empty())
            return writeError(http::status::not_found, "No CDN could fulfill this request.");

        // Note: body implementation is a shared_ptr so that it can be ... you guessed it, shared!
        auto parserState = std::make_shared<beast::user::BlockTableEncodedStreamTransform>([&](std::span<uint8_t const> data) {
            // Call asio::write instead of beast, because we want the data to get out instantly.
            boost::asio::write(_stream.socket(), boost::asio::buffer(data.data(), data.size()));
        }, [&](std::size_t bytesRead) {
            // Update remote range header; This will make sure we currectly resume from another CDN if the inflight one fails.
            params.Offset += bytesRead;
            params.Length -= bytesRead;
        });

        for (auto& [cdn, remotePath] : availableRemoteArchives) {
            beast::tcp_stream remoteStream { _stream.get_executor() };
            remoteStream.connect(resolver.resolve(cdn, "80"), ec);
            if (ec.failed())
                continue;

            http::request<http::empty_body> remoteRequest { http::verb::get, remotePath, 11 };
            remoteRequest.set(http::field::host, cdn);
            if (params.Length != 0)
                remoteRequest.set(http::field::range, fmt::format("{}-{}", params.Offset, params.Offset + params.Length - 1));

            http::write(remoteStream, remoteRequest, ec);
            if (ec.failed())
                continue;

            http::response_parser<beast::user::blte_body> remoteResponse;
            remoteResponse.body_limit({ }); // TODO: Use this actually to request chunks of data from Blizzard and do that in fixed intervals
                                            //       (will maybe reduce overhead on Blizzard's side .... /s)
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

    std::unordered_map<std::string_view, std::string> Session::CollectAvailableCDNs(libtactmon::ribbit::types::CDNs& cdns, std::string_view archiveName, std::size_t offset, std::size_t length) {
        utility::ThreadPool workers{ 4 };

        using result_type = std::pair<std::string_view, std::string>;

        std::list<boost::future<std::optional<result_type>>> archiveFutures;
        
        for (ribbit::types::cdns::Record const& cdn : cdns) {
            std::string remotePath = fmt::format("/{}/data/{}/{}/{}", cdn.Path, archiveName.substr(0, 2), archiveName.substr(2, 2), archiveName);

            for (std::string_view host : cdn.Hosts) {
                auto validationTask = std::make_shared<boost::packaged_task<std::optional<result_type>>>([&, remotePath, host]() -> std::optional<result_type> {
                    beast::error_code ec;

                    auto strand = boost::asio::make_strand(workers.executor());

                    boost::asio::ip::tcp::resolver resolver(strand);
                    beast::tcp_stream remoteStream(strand);
                    remoteStream.connect(resolver.resolve(host, "80"), ec);
                    if (ec.failed())
                        return std::nullopt;

                    auto checkExistence = [&](http::verb method) -> bool
                    {
                        http::request<http::empty_body> remoteRequest(method, remotePath, 11);
                        remoteRequest.set(http::field::host, host);
                        if (length != 0)
                            remoteRequest.set(http::field::range, fmt::format("{}-{}", offset, offset + length - 1));

                        http::write(remoteStream, remoteRequest, ec);
                        if (ec.failed())
                            return false;

                        beast::flat_buffer buffer { 1024 };
                        http::response_parser<http::dynamic_body> response;
                        response.body_limit({ });
                        http::read_header(remoteStream, buffer, response, ec);
                        if (ec.failed())
                            return false;

                        switch (response.get().result()) {
                            case http::status::not_found:
                            case http::status::range_not_satisfiable:
                            case http::status::method_not_allowed:
                                return false;
                            default:
                                break;
                        }

                        return true;
                    };

                    if (checkExistence(http::verb::head) || checkExistence(http::verb::get))
                        return std::pair { host, remotePath };

                    return std::nullopt;
                });

                archiveFutures.push_back(validationTask->get_future());

                boost::asio::post(workers.pool_executor(), [validationTask]() { (*validationTask)(); });
            }
        }

        std::unordered_map<std::string_view, std::string> resultSet;
        for (boost::future<std::optional<result_type>>& future : boost::when_all(archiveFutures.begin(), archiveFutures.end()).get()) {
            std::optional<result_type> value = future.get();
            if (!value.has_value())
                continue;

            resultSet.emplace(value->first, value->second);
        }

        return resultSet;
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
            beast::async_write(_stream, std::move(msg), std::bind_front(&Session::HandleWrite, this->shared_from_this(), keepAlive));
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


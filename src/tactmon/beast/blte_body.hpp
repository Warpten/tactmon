#pragma once

#include "beast/blte_stream_reader.hpp"

#include <cstdint>
#include <functional>

#include <boost/asio/buffer.hpp>
#include <boost/beast/http.hpp>

namespace boost::beast::user {
    struct blte_body {
        struct value_type {
            void open(std::function<void(uint8_t*, size_t)> acceptor) {
                _acceptor = acceptor;
            }

            std::function<void(uint8_t*, size_t)> _acceptor;
        };

        struct writer {
            using const_buffer_type = boost::asio::const_buffer;

            template <bool IsRequest, typename Fields>
            writer(boost::beast::http::header<IsRequest, Fields> const& header, value_type const& body)
            {

            }

            void init(boost::system::error_code& ec) {
                ec = { };
            }

            boost::optional<std::pair<const_buffer_type, bool>> get(boost::system::error_code& ec) { return { }; }
        };

        struct reader {
            template <bool IsRequest, typename Fields>
            reader(boost::beast::http::header<IsRequest, Fields>& header, value_type& body)
                : _body(body), _reader()
            {

            }

            void init(boost::optional<uint64_t> const& contentLength, boost::system::error_code& ec) {
                ec = { };
            }

            template <typename ConstBufferSequence>
            std::size_t put(ConstBufferSequence const& buffers, boost::system::error_code& ec) {
                return _reader.write_some((uint8_t*) buffers.data(), buffers.size(), _body._acceptor, ec);
            }

            void finish(boost::system::error_code& ec) { }

        private:
            value_type& _body;
            blte_stream_reader _reader;
        };
    };
}

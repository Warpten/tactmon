#pragma once

#include "beast/blte_stream_reader.hpp"

#include <cstdint>
#include <functional>

#include <boost/asio/buffer.hpp>
#include <boost/beast/http.hpp>

namespace boost::beast::user {
    struct blte_body {
        struct value_type_handle {
            using Handler = std::function<void(std::span<uint8_t const>)>;

            Handler handler;
            blte_stream_reader reader;
        };
        using value_type = std::shared_ptr<value_type_handle>;

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
                : _body(body)
            {

            }

            void init(boost::optional<uint64_t> const& contentLength, boost::system::error_code& ec) {
                ec = { };
            }

            template <typename ConstBufferSequence>
            std::size_t put(ConstBufferSequence const& buffers, boost::system::error_code& ec) {
                return _body->reader.write_some((uint8_t*) buffers.data(), buffers.size(), _body->handler, ec);
            }

            void finish(boost::system::error_code& ec) { }

        private:
            value_type& _body;
        };
    };
}

#pragma once

#include "beast/BlockTableEncodedStreamTransform.hpp"

#include <cstdint>
#include <functional>

#include <boost/asio/buffer.hpp>
#include <boost/beast/http.hpp>

namespace boost::beast::user {
    struct blte_body {
        using value_type = std::shared_ptr<BlockTableEncodedStreamTransform>;

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
                return _body->Parse((uint8_t*) buffers.data(), buffers.size(), ec);
            }

            void finish(boost::system::error_code& ec) { }

        private:
            value_type& _body;
        };
    };
}

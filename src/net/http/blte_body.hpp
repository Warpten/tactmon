#pragma once

#include <boost/asio/buffer.hpp>
#include <boost/beast/http/message.hpp>
#include <boost/optional.hpp>

#include "io/mem/MemoryStream.hpp"
#include "tact/EKey.hpp"

namespace net::http {
	template <typename Target>
	struct blte_body {
		using value_type = typename Target::value_type;

		blte_body(tact::EKey const& ekey) : _ekey(ekey) { }

		struct reader {
			template <bool IsRequest, typename Fields>
			reader(boost::beast::http::header<IsRequest, Fields>& header, value_type& body) {

			}

			void init(boost::optional<std::uint64_t> const&, boost::error_code& ec) {

			}

			template <typename ConstBufferSequence>
			std::size_t put(ConstBufferSequence const& buffers, boost::error_code& ec) {
				std::span<uint8_t const> data { buffers.data(), buffers.size() };

				_stream.Write(data, std::endian::native);
			}

			void finish(boost::error_code& ec) {

			}

			io::mem::MemoryStream _stream;
		};

	private:
		value_type _body;
		tact::EKey _ekey;
	};
}
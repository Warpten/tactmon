#pragma once

#include <cstdint>

#include <boost/optional.hpp>
#include <boost/system/error_code.hpp>

namespace net::http {
	template <typename... Ts>
	struct multi_body {
		struct reader {
			void init(boost::optional<uint64_t> const& sz, boost::error_code const& ec) {
				
			}

			template <typename ConstBufferSequence>
			std::size_t put(ConstBufferSequence const& buffers, boost::system::error_code& ec) {
				return 0;
			}

			void finish(boost::system::error_code& ec) {

			}
		};
	};
}
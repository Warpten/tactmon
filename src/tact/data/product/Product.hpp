#pragma once

#include "io/LocalCache.hpp"

#include <boost/asio/io_context.hpp>

#include <optional>
#include <string_view>

namespace tact::data::product {
	struct Product {
		explicit Product(std::string_view productName, boost::asio::io_context& context);

		bool Refresh() noexcept;

		// TODO: API to download files

	protected:
		virtual bool LoadRoot() = 0;

	private:
		boost::asio::io_context& _context;
		std::string _productName;

	protected:
		std::optional<io::LocalCache> _localInstance;
	};
}
#pragma once

#include "tact/data/product/Product.hpp"

#include <functional>
#include <memory>
#include <string>
#include <string_view>
#include <unordered_map>

#include <boost/asio/io_context.hpp>

namespace tact::data::product {
    /**
     * Manages various products.
     */
    struct Manager final {
        explicit Manager(boost::asio::io_context& context);

        /**
         * Registers a product as returned by the provided lambda.
         * 
         * @param[in] factory A lambda returning an instance of the product handler.
         */
        void Register(std::string const& product, std::function<std::shared_ptr<tact::data::product::Product>(boost::asio::io_context&)> factory);

        /**
         * Returns an object allowing access to a product's files on Blizzard's CDNs.
         * 
         * @param[in] productName The name of the product of interest.
         * 
         * @returns An instance of @ref tact::data::product::Product, or @code nullptr if the product had not been previously registered.
         */
        std::shared_ptr<tact::data::product::Product> Rent(std::string const& productName) const;

    private:
        boost::asio::io_context& _context;
        std::unordered_map<std::string, std::function<std::shared_ptr<tact::data::product::Product>()>> _productFactories;
    };
}

#pragma once

#include "backend/Product.hpp"
#include "backend/db/entity/Build.hpp"

#include <chrono>
#include <functional>
#include <list>
#include <memory>
#include <string>
#include <unordered_map>

#include <boost/asio/any_io_executor.hpp>
#include <boost/asio/high_resolution_timer.hpp>
#include <boost/container/stable_vector.hpp>

namespace backend {
    using namespace std::chrono_literals;

    struct ProductCache final {
        explicit ProductCache(boost::asio::any_io_executor executor);

    public:

        /**
         * Loads a given product configuration.
         * 
         * @param[in] productName   The name of the product to load
         * @param[in] configuration An instance of a build entity.
         * @param[in] handler       A function that will be called when the product is done loading.
         * @param[in] lifetime      The maximum time this product will live in memory.
         */
        bool LoadConfiguration(std::string productName, db::entity::build::Entity const& configuration, std::function<void(Product&)> handler,
            std::chrono::high_resolution_clock::duration lifetime = 15min);

        /**
         * Registers a factory function for a product.
         * 
         * @param[in] productName The name of the product for which a factory will be registered.
         * @param[in] factory     The factory function.
         * 
         * @remarks This function will override any previously registered factory.
         */
        void RegisterFactory(std::string productName, std::function<Product()> factory);

        /**
         * Iterates over each loaded product.
         * 
         * @param[in] handler A function that will be called with the product instance, and its expiration time.
         */
        void ForEachProduct(std::function<void(Product&, std::chrono::high_resolution_clock::time_point)> handler);

        /**
         * Returns the amount of loaded products.
         */
        [[nodiscard]] std::size_t size() const { return _products.size(); }

        bool IsAwareOf(std::string const& productName) const;

    private:
        void RemoveExpiredEntries();

        struct Record {
            Record(Product product, std::chrono::high_resolution_clock::time_point expirationTimer);

            Product product;
            std::chrono::high_resolution_clock::time_point expirationTimer;
        };

        boost::asio::high_resolution_timer _cacheExpiryTimer; //< Executes every minute.
        std::list<std::shared_ptr<Record>> _products; //< Currently managed products
        std::unordered_map<std::string, std::function<Product()>> _productFactories; //< Functions that instanciate product managers.
    };
}

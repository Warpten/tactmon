#pragma once

#include "backend/db/entity/Build.hpp"

#include <atomic>
#include <chrono>
#include <functional>
#include <list>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include <boost/asio/io_context_strand.hpp>
#include <boost/asio/high_resolution_timer.hpp>
#include <boost/container/stable_vector.hpp>

#include <tact/data/product/Product.hpp>

namespace backend {
    struct Product final : std::enable_shared_from_this<Product> {
        explicit Product() { }
        Product(Product&& other) noexcept;

        Product& operator = (Product&& other) noexcept;

        explicit Product(std::shared_ptr<tact::data::product::Product> product);

        db::entity::build::Entity const& GetLoadedBuild() const { return _currentBuild; }

        void AddListener(std::function<void(Product&)> callback);
        bool Load(db::entity::build::Entity const& entity);

        tact::data::product::Product* operator -> () { return _product.get(); }

    private:
        db::entity::build::Entity _currentBuild;
        std::shared_ptr<tact::data::product::Product> _product;

        std::vector<std::function<void(Product&)>> _callbacks;
        std::atomic_bool _loading;
    };

    struct ProductCache final {
        explicit ProductCache(boost::asio::io_context::strand cacheStrand);

        bool LoadConfiguration(std::string productName, db::entity::build::Entity const& configuration, std::function<void(Product&)> handler);
        void RegisterFactory(std::string productName, std::function<Product()> factory);
        void ForEachProduct(std::function<void(Product&, std::chrono::high_resolution_clock::time_point)> handler);

        size_t size() const { return _products.size(); }

    private:
        void RemoveExpiredEntries();

        struct Record {
            Record(Product product, std::chrono::high_resolution_clock::time_point expirationTimer);

            Product product;
            std::chrono::high_resolution_clock::time_point expirationTimer;
        };

        boost::asio::io_context::strand _cacheStrand;
        boost::asio::high_resolution_timer _expirationTimer;
        // We **want** iterator stability above all else
#if 0
        boost::container::stable_vector<Record> _products;
#else
        std::list<std::shared_ptr<Record>> _products;
#endif
        std::unordered_map<std::string, std::function<Product()>> _productFactories;
    };
}

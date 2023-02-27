#include "backend/ProductCache.hpp"

#include <boost/range/adaptor/map.hpp>

using namespace std::chrono_literals;

namespace backend {
    ProductCache::ProductCache(boost::asio::any_io_executor executor)
        : _cacheExpiryTimer(executor)
    {
        RemoveExpiredEntries();
    }

    void ProductCache::RemoveExpiredEntries() {
        _products.remove_if([](std::shared_ptr<Record> record) { return record->expirationTimer <= std::chrono::high_resolution_clock::now(); });

        _cacheExpiryTimer.expires_at(std::chrono::high_resolution_clock::now() + 1min);
        _cacheExpiryTimer.async_wait([this](boost::system::error_code ec) {
            if (ec != boost::asio::error::operation_aborted)
            this->RemoveExpiredEntries();
        });
    }

    bool ProductCache::LoadConfiguration(std::string productName, db::entity::build::Entity const& configuration, std::function<void(Product&)> handler) {
        for (std::shared_ptr<Record> loadedProduct : _products) {
            db::entity::build::Entity const& loadedConfiguration = loadedProduct->product.GetLoadedBuild();
            if (db::get<db::entity::build::id>(loadedConfiguration) != db::get<db::entity::build::id>(configuration))
                continue;

            // Reinitialize expiry timer if this build is requested
            loadedProduct->expirationTimer = std::chrono::high_resolution_clock::now() + 15min;

            handler(loadedProduct->product);
            return true;
        }

        auto factoryItr = _productFactories.find(productName);
        if (factoryItr == _productFactories.end())
            return false;

        auto itr = _products.insert(_products.end(), std::make_shared<Record>(factoryItr->second(), std::chrono::high_resolution_clock::now() + 15min));
        (*itr)->product.AddListener(handler);
        return (*itr)->product.Load(configuration);
    }

    void ProductCache::RegisterFactory(std::string productName, std::function<Product()> factory) {
        _productFactories.emplace(productName, factory);
    }

    void ProductCache::ForEachProduct(std::function<void(Product&, std::chrono::high_resolution_clock::time_point)> handler) {
        for (std::shared_ptr<Record> record : _products)
            handler(record->product, record->expirationTimer);
    }

    bool ProductCache::IsAwareOf(std::string const& productName) const {
        return _productFactories.find(productName) != _productFactories.end();
    }

    ProductCache::Record::Record(Product product, std::chrono::high_resolution_clock::time_point expirationTimer)
        : product(std::move(product)), expirationTimer(expirationTimer)
    {

    }
}

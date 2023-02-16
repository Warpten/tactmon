#include "backend/Product.hpp"

#include <chrono>

namespace backend {
    using namespace std::chrono_literals;

    Product::Product(std::shared_ptr<tact::data::product::Product> product) : _product(product), _loading(false) {

    }

    Product::Product(Product&& other) noexcept
        : _product(std::move(other._product)), _loading(false), _currentBuild(std::move(other._currentBuild)), _callbacks(std::move(other._callbacks))
    { }

    Product& Product::operator = (Product&& other) noexcept {
        _product = std::move(other._product);
        _loading.store(other._loading.load());
        _currentBuild = std::move(other._currentBuild);
        _callbacks = std::move(other._callbacks);

        return *this;
    }

    ProductCache::ProductCache(boost::asio::io_context::strand cacheStrand)
        : _cacheStrand(cacheStrand), _expirationTimer(cacheStrand.context())
    {
        RemoveExpiredEntries();
    }

    void ProductCache::RemoveExpiredEntries() {
        _products.remove_if([](std::shared_ptr<Record> record) { return record->expirationTimer <= std::chrono::high_resolution_clock::now(); });

        _expirationTimer.expires_at(std::chrono::high_resolution_clock::now() + 1min);
        _expirationTimer.async_wait([this](boost::system::error_code ec) {
            if (ec != boost::asio::error::operation_aborted)
                this->RemoveExpiredEntries();
        });
    }

    void Product::AddListener(std::function<void(Product&)> callback) {
        _callbacks.emplace_back(callback);
    }

    bool Product::Load(db::entity::build::Entity const& entity) {
        if (db::get<db::entity::build::id>(entity) != db::get<db::entity::build::id>(_currentBuild)) {
            _currentBuild = entity;

            if (!_loading.exchange(true)) {
                if (!_product->Load(db::get<db::entity::build::build_config>(entity), db::get<db::entity::build::cdn_config>(entity)))
                    return false;
            }
        }

        for (auto&& listener : _callbacks)
            listener(*this);

        return true;
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

    ProductCache::Record::Record(Product product, std::chrono::high_resolution_clock::time_point expirationTimer)
        : product(std::move(product)), expirationTimer(expirationTimer)
    {

    }
}

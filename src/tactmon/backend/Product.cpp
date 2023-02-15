#include "backend/Product.hpp"

namespace backend {
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

    void Product::AddListener(std::function<void(Product&)> callback) {
        _callbacks.emplace_back(callback);
    }

    bool Product::Load(db::entity::build::Entity const& entity) {
        if (db::get<db::entity::build::id>(entity) != db::get<db::entity::build::id>(_currentBuild)) {
            // _product = entity; // TODO FIXME

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
        for (Product& loadedProduct : _products) {
            db::entity::build::Entity const& loadedConfiguration = loadedProduct.GetLoadedBuild();
            if (db::get<db::entity::build::id>(loadedConfiguration) != db::get<db::entity::build::id>(configuration))
                continue;

            handler(loadedProduct);
            return true;
        }

        auto factoryItr = _productFactories.find(productName);
        // TODO: assert the iterator is valid

        Product& instance = _products.emplace_back(factoryItr->second());
        instance.AddListener(handler);
        return instance.Load(configuration);
    }

    void ProductCache::RegisterFactory(std::string productName, std::function<Product()> factory) {
        _productFactories.emplace(productName, factory);
    }
}

#pragma once

#include "backend/db/entity/Build.hpp"

#include <atomic>
#include <functional>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

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
        explicit ProductCache() = default;

        
        bool LoadConfiguration(std::string productName, db::entity::build::Entity const& configuration, std::function<void(Product&)> handler);

        void RegisterFactory(std::string productName, std::function<Product()> factory);

    private:
        std::vector<Product> _products;
        std::unordered_map<std::string, std::function<Product()>> _productFactories;
    };
}

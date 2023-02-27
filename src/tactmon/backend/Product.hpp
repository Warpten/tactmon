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

#include <boost/asio/execution_context.hpp>
#include <boost/asio/high_resolution_timer.hpp>
#include <boost/container/stable_vector.hpp>

#include <libtactmon/tact/data/product/Product.hpp>

namespace backend {
    struct Product final : std::enable_shared_from_this<Product> {
        explicit Product() { }
        Product(Product&& other) noexcept;

        Product& operator = (Product&& other) noexcept;

        explicit Product(std::shared_ptr<libtactmon::tact::data::product::Product> product);

        db::entity::build::Entity const& GetLoadedBuild() const { return _currentBuild; }

        void AddListener(std::function<void(Product&)> callback);
        bool Load(db::entity::build::Entity const& entity);

        libtactmon::tact::data::product::Product* operator -> () { return _product.get(); }

    private:
        db::entity::build::Entity _currentBuild;
        std::shared_ptr<libtactmon::tact::data::product::Product> _product;

        std::vector<std::function<void(Product&)>> _callbacks;
        std::atomic_bool _loading;
    };


}

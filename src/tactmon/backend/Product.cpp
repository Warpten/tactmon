#include "backend/Product.hpp"

#include <chrono>

namespace backend {
    using namespace std::chrono_literals;

    Product::Product(std::shared_ptr<libtactmon::tact::data::product::Product> product) : _product(product), _loading(false) {

    }

    Product::Product(Product&& other) noexcept
        : _currentBuild(std::move(other._currentBuild)), _product(std::move(other._product)), _loading(false)
    { }

    Product& Product::operator = (Product&& other) noexcept {
        _product = std::move(other._product);
        _loading.store(other._loading.load());
        _currentBuild = std::move(other._currentBuild);

        return *this;
    }

    bool Product::Load(db::entity::build::Entity const& entity) {
        if (db::get<db::entity::build::id>(entity) != db::get<db::entity::build::id>(_currentBuild)) {
            _currentBuild = entity;

            if (!_loading.exchange(true)) {
                if (!_product->Load(db::get<db::entity::build::build_config>(entity), db::get<db::entity::build::cdn_config>(entity)))
                    return false;
            }
        }

        return true;
    }
}

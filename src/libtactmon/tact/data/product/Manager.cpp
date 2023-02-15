#include "tact/data/product/Manager.hpp"
#include "tact/data/product/wow/Product.hpp"

namespace tact::data::product {
    void Manager::Register(std::string product, std::function<std::shared_ptr<tact::data::product::Product>()> factory) {
        _productFactories.emplace(product, [=]() { return factory(); });
    }

    std::shared_ptr<tact::data::product::Product> Manager::Rent(std::string const& productName) const {
        auto itr = _productFactories.find(productName);
        if (itr == _productFactories.end())
            return nullptr;

        return itr->second();
    }
}

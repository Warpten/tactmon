#include "tact/data/product/Manager.hpp"
#include "tact/data/product/wow/Product.hpp"

namespace tact::data::product {
    Manager::Manager(boost::asio::io_context& context) : _context(context) { }

    void Manager::Register(std::string const& product, std::function<std::shared_ptr<tact::data::product::Product>(boost::asio::io_context&)> factory) {
        _productFactories.emplace(product, [=]() { return factory(_context); });
    }

    std::shared_ptr<tact::data::product::Product> Manager::Rent(std::string const& productName) const {
        auto itr = _productFactories.find(productName);
        if (itr == _productFactories.end())
            return nullptr;

        return itr->second();
    }
}

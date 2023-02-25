#pragma once

#include "libtactmon/tact/data/product/Product.hpp"
#include "libtactmon/tact/data/product/wow/Root.hpp"

#include <array>
#include <cstdint>
#include <optional>
#include <vector>

namespace libtactmon::tact::data::product::wow {
    /**
     * An implementation of World of Warcraft related game products.
     */
    struct Product final : tact::data::product::Product {
        using Base = tact::data::product::Product;
        using Base::Base;

        std::optional<tact::data::FileLocation> FindFile(std::string_view fileName) const override;
        std::optional<tact::data::FileLocation> FindFile(uint32_t fileDataID) const override;

        bool Load(std::string_view buildConfig, std::string_view cdnConfig) noexcept override;

    private:
        std::optional<tact::data::product::wow::Root> _root;
    };
}

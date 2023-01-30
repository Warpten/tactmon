#pragma once

#include "tact/data/product/Product.hpp"

#include <array>
#include <cstdint>
#include <vector>

namespace tact::data::product::wow {
    struct Product final : tact::data::product::Product {
        using tact::data::product::Product::Product;

        struct Entry final {
            Entry(std::span<uint8_t, 0x10> hash, uint64_t fileDataID, uint64_t nameHash) noexcept
                :  _fileDataID(fileDataID), _nameHash(nameHash)
            {
                std::copy_n(hash.begin(), 0x10, _hash.begin());
            }

            std::array<uint8_t, 0x10> _hash { 0 };
            uint64_t _fileDataID = 0;
            uint64_t _nameHash = 0;
        };

    protected:
        bool LoadRoot() override;

    private:
        std::vector<Entry> _entries;
    };
}
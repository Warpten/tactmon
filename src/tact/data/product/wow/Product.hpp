#pragma once

#include "tact/data/product/Product.hpp"
#include "tact/data/product/wow/Root.hpp"

#include <array>
#include <cstdint>
#include <optional>
#include <vector>

namespace tact::data::product::wow {
    struct Product final : tact::data::product::Product {
        using Base = tact::data::product::Product;
        using Base::Base;

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

        std::optional<tact::data::FileLocation> FindFile(std::string_view fileName) const override;
        std::optional<tact::data::FileLocation> FindFile(uint32_t fileDataID) const override;

        bool Refresh() noexcept override;

    private:
        std::vector<Entry> _entries;

        std::optional<tact::data::product::wow::Root> _root;
    };
}

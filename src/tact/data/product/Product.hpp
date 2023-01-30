#pragma once

#include "io/LocalCache.hpp"
#include "tact/BLTE.hpp"
#include "tact/CKey.hpp"
#include "tact/data/FileLocation.hpp"

#include <boost/asio/io_context.hpp>

#include <cstdint>
#include <optional>
#include <string_view>

namespace tact::data::product {
    struct Product {
        explicit Product(std::string_view productName, boost::asio::io_context& context);

        bool Refresh() noexcept;

        virtual std::optional<tact::data::FileLocation> FindFile(std::string_view fileName) const { return std::nullopt; }
        virtual std::optional<tact::data::FileLocation> FindFile(uint32_t fileDataID) const { return std::nullopt; }
        std::optional<tact::data::FileLocation> FindFile(tact::CKey const& contentKey) const;

        std::optional<tact::BLTE> Open(tact::data::FileLocation const& location) const;

    protected:
        virtual bool LoadRoot() = 0;

    private:
        boost::asio::io_context& _context;
        std::string _productName;

    protected:
        std::optional<io::LocalCache> _localInstance;
    };
}

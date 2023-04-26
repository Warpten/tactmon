#include "libtactmon/tact/data/product/wow/Product.hpp"
#include "libtactmon/utility/Formatting.hpp"

#include <fstream>

#include <fmt/chrono.h>

#include <spdlog/stopwatch.h>

namespace libtactmon::tact::data::product::wow {
    bool Product::Load(std::string_view buildConfig, std::string_view cdnConfig) noexcept {
        if (!tact::data::product::Product::Load(buildConfig, cdnConfig))
            return false;

        std::optional<tact::data::FileLocation> rootLocation = Base::FindFile(_buildConfig->Root);
        if (!rootLocation)
            return false;

        spdlog::stopwatch sw;

        Result<tact::data::product::wow::Root> root = [&]() {
            for (std::size_t i = 0; i < rootLocation->keyCount(); ++i) {
                tact::EKey key = (*rootLocation)[i];

                auto root = Base::ResolveCachedBLTE(key).transform([&encoding = _encoding](io::GrowableMemoryStream decompressedStream) {
                    return tact::data::product::wow::Root::Parse(decompressedStream, encoding->GetContentKeySize());
                });
                if (root.has_value())
                    return root;
            }

            return Result<tact::data::product::wow::Root> { Error::RootManifestNotFound };
        }();

        if (!root.has_value()) {
            if (_logger != nullptr)
                _logger->error("({}) An error occured while loading the root manifest: {}.", _buildConfig->BuildName, root.code());

            return false;
        }

        _root = std::move(root).ToOptional();

        if (_logger != nullptr)
            _logger->info("({}) Root manifest loaded in {:.3} seconds ({} entries).", _buildConfig->BuildName, sw, _root->size());
        return true;
    }

    std::optional<tact::data::FileLocation> Product::FindFile(std::string_view fileName) const {
        std::optional<tact::data::FileLocation> parentResult = Base::FindFile(fileName);
        if (parentResult.has_value())
            return parentResult;

        std::optional<tact::CKey> contentKey = [&]() -> std::optional<tact::CKey> {
            // Try in root (which would use jenkins96 hash)
            if (_root.has_value()) {
                std::optional<tact::CKey> contentKey = _root->FindFile(fileName);
                if (contentKey.has_value())
                    return contentKey;
            }

            return std::nullopt;
        }();

        if (!contentKey.has_value())
            return std::nullopt;
        
        // And finally resolve the CKey we found.
        return Base::FindFile(*contentKey);
    }

    std::optional<tact::data::FileLocation> Product::FindFile(uint32_t fileDataID) const {
        if (_root.has_value()) {
            std::optional<tact::CKey> contentKey = _root->FindFile(fileDataID);
            if (contentKey.has_value())
                return Base::FindFile(*contentKey);
        }

        return std::nullopt;
    }
}

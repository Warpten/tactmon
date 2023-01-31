#include "tact/BLTE.hpp"
#include "tact/data/product/wow/Product.hpp"
#include "io/LocalCache.hpp"
#include "io/net/Download.hpp"

#include <fstream>

namespace tact::data::product::wow {
    bool Product::LoadRoot() {
        auto&& buildConfig = _localInstance->GetBuildConfig();
        if (!buildConfig.has_value())
            return false;

        std::optional<tact::data::FileLocation> rootLocation = _localInstance->FindFile(buildConfig->Root);
        if (!rootLocation)
            return false;

        std::optional<tact::BLTE> stream = Open(*rootLocation);
        if (!stream.has_value())
            return false;

        _root.emplace(stream->GetStream(), _localInstance->GetContentKeySize());

        _localInstance->GetLogger()->info("Found {} entries in root manifest.", _root->size());
        return true;
    }

    std::optional<tact::data::FileLocation> Product::FindFile(std::string_view fileName) const {
        if (_root.has_value()) {
            std::optional<tact::CKey> contentKey = _root->FindFile(fileName);
            if (!contentKey.has_value())
                return std::nullopt;

            return tact::data::product::Product::FindFile(*contentKey);
        }

        std::optional<tact::CKey> contentKey = _localInstance->FindFile(fileName);
        if (!contentKey.has_value())
            return std::nullopt;
        
        return tact::data::product::Product::FindFile(*contentKey);
    }

    std::optional<tact::data::FileLocation> Product::FindFile(uint32_t fileDataID) const {
        if (!_root.has_value())
            return std::nullopt;

        std::optional<tact::CKey> contentKey = _root->FindFile(fileDataID);
        if (!contentKey.has_value())
            return std::nullopt;

        return tact::data::product::Product::FindFile(*contentKey);
    }
}

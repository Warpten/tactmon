#include "tact/BLTE.hpp"
#include "tact/data/product/wow/Product.hpp"

#include <fstream>

namespace tact::data::product::wow {
    bool Product::Refresh() noexcept {
        if (!tact::data::product::Product::Refresh())
            return false;

        std::optional<tact::data::FileLocation> rootLocation = Base::FindFile(_buildConfig->Root);
        if (!rootLocation)
            return false;

        std::optional<tact::data::product::wow::Root> root = Base::ResolveCachedData<tact::data::product::wow::Root>(_buildConfig->Root.ToString(), [&encoding = _encoding](io::FileStream& fstream) -> std::optional<tact::data::product::wow::Root> {
            std::optional<tact::BLTE> blte = tact::BLTE::Parse(fstream);
            if (blte.has_value())
                return tact::data::product::wow::Root { blte->GetStream(), encoding->GetContentKeySize() };
            
            return std::nullopt;
        });

        if (!root.has_value())
            return false;

        _root.emplace(std::move(*root));

        _logger->info("Found {} entries in root manifest.", _root->size());
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

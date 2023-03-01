#include "libtactmon/tact/BLTE.hpp"
#include "libtactmon/tact/data/product/wow/Product.hpp"

#include <fstream>

namespace libtactmon::tact::data::product::wow {
    bool Product::Load(std::string_view buildConfig, std::string_view cdnConfig) noexcept {
        if (!tact::data::product::Product::Load(buildConfig, cdnConfig))
            return false;

        std::optional<tact::data::FileLocation> rootLocation = Base::FindFile(_buildConfig->Root);
        if (!rootLocation)
            return false;

        _root = [&]() -> std::optional<tact::data::product::wow::Root> {
            for (size_t i = 0; i < rootLocation->keyCount(); ++i) {
                tact::CKey key{ (*rootLocation)[i] };

                auto root = Base::ResolveCachedData<tact::data::product::wow::Root>(key.ToString(), [&encoding = _encoding](io::IReadableStream& fstream) -> std::optional<tact::data::product::wow::Root> {
                    std::optional<tact::BLTE> blte = tact::BLTE::Parse(fstream);
                    if (blte.has_value())
                        return tact::data::product::wow::Root { blte->GetStream(), encoding->GetContentKeySize() };

                    return std::nullopt;
                });
                if (root.has_value())
                    return root;
            }

            return std::nullopt;
        }();

        if (!_root.has_value())
            return false;

        if (_logger != nullptr)
            _logger->info("({}) {} entries found in root manifest.", _buildConfig->BuildName, _root->size());
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

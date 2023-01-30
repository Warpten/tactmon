#include "tact/data/product/wow/Product.hpp"
#include "io/LocalCache.hpp"
#include "io/net/Download.hpp"

namespace tact::data::product::wow {
    bool Product::LoadRoot() {
        auto&& buildConfig = _localInstance->GetBuildConfig();
        if (!buildConfig.has_value())
            return false;

        std::optional<tact::data::FileLocation> rootLocation = _localInstance->FindFile(buildConfig->Root);
        if (!rootLocation)
            return false;

        for (size_t i = 0; i < rootLocation->keyCount(); ++i) {
            std::span<const uint8_t> encodingKey = (*rootLocation)[i];

            // We don't use tact::EKey and tact::CKey internally, so jump through some hoops
            // (This is probably a design flaw; all EKey and CKey should be is probably std::span in disguise)
            
            // TODO: Read root here
            // io::net::Download expects a CKey, which isn't great
        }
        return true;
    }
}

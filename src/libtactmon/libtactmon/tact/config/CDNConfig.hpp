#pragma once

#include <cstdint>
#include <functional>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace libtactmon::io {
    struct IReadableStream;
}

namespace libtactmon::tact::config {
    /**
     * Represents a CDN configuration.
     * 
     * Does **not** model all properties **yet** (mostly because patch-archives are not really understood).
     */
    struct CDNConfig final {
        struct Archive {
            std::string Name;
            size_t Size = 0;
        };

        static std::optional<CDNConfig> Parse(io::IReadableStream& stream);

    private:
        CDNConfig() = default;
        
    public:
        std::vector<Archive> archives;
        std::optional<CDNConfig::Archive> fileIndex;
    };
}

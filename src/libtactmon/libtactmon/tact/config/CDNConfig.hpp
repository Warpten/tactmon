#pragma once

#include <cstdint>
#include <functional>
#include <string>
#include <string_view>
#include <vector>

namespace libtactmon::io {
    struct IReadableStream;
}

namespace libtactmon::tact::config {
    struct CDNConfig final {
        struct Archive {
            Archive(std::string_view archiveName) : Name(archiveName) { }

            std::string Name;
            size_t Size;
        };

        explicit CDNConfig(io::IReadableStream& stream);

        void ForEachArchive(std::function<void(std::string_view, size_t)> handler);

    private:
        std::vector<Archive> _archives;
    };
}

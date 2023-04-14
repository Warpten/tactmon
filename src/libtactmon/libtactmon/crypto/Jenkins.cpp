#include "libtactmon/crypto/Jenkins.hpp"
#include "libtactmon/crypto/lookup3.hpp"

#include <algorithm>

namespace libtactmon::crypto {
    uint32_t JenkinsHash(std::string_view path) {
        std::string normalizedPath { path };
        std::transform(normalizedPath.begin(), normalizedPath.end(), normalizedPath.begin(), [](char c) {
            if (c >= 'a' && c <= 'z')
                return static_cast<char>(c + 'A' - 'a');
            return c;
        });
        std::replace(normalizedPath.begin(), normalizedPath.end(), '/', '\\');

        uint32_t pc = 0;
        uint32_t pb = 0;
        hashlittle2(normalizedPath.data(), normalizedPath.size(), &pc, &pb);
        return pc;
    }
}

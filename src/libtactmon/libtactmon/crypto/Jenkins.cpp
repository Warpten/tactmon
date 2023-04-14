#include "libtactmon/crypto/Jenkins.hpp"
#include "libtactmon/crypto/lookup3.hpp"

#include <string>

namespace libtactmon::crypto {
    uint32_t JenkinsHash(std::string_view path) {
        std::string normalizedPath { path };

        for (std::string::value_type& c : normalizedPath) {
            if (c == '/')
                c = '\\';
            else if (c >= 'a' && c <= 'z')
                c += 'A' - 'a';
        }

        uint32_t pc = 0;
        uint32_t pb = 0;
        hashlittle2(normalizedPath.data(), normalizedPath.size(), &pc, &pb);
        return pc;
    }
}

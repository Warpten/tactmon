#include "libtactmon/crypto/Jenkins.hpp"
#include "libtactmon/crypto/lookup3.hpp"

#include <boost/algorithm/string/case_conv.hpp>

namespace libtactmon::crypto {
    uint32_t JenkinsHash(std::string_view path) {
        std::string normalizedPath{ path };
        boost::algorithm::to_upper(normalizedPath);
        std::replace(normalizedPath.begin(), normalizedPath.end(), '/', '\\');

        uint32_t pc = 0;
        uint32_t pb = 0;
        hashlittle2(normalizedPath.data(), normalizedPath.size(), &pc, &pb);
        return pc;
    }
}

#pragma once

#include <cstdint>
#include <string_view>

namespace libtactmon::crypto {
    uint32_t JenkinsHash(std::string_view path);
}

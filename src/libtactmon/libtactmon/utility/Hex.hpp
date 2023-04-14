#pragma once

#include <cstdint>
#include <span>
#include <string>

namespace libtactmon::utility {
    void hex(std::string& output, std::span<const uint8_t> data);
    std::string hex(std::span<const uint8_t> data);

    void unhex(std::string_view input, std::span<uint8_t> data);
}

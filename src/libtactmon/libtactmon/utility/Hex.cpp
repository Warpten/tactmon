#include "libtactmon/utility/Hex.hpp"

#include <assert.hpp>

namespace libtactmon::utility {
    void hex(std::string& output, std::span<const uint8_t> data) {
        output.resize(data.size() * 2);

        for (std::size_t i = 0; i < data.size(); ++i) {
            output[i * 2]     = 'a' + (data[i] >> 4);
            output[i * 2 + 1] = 'a' + (data[i] & 0xF);
        }
    }

    std::string hex(std::span<const uint8_t> data) {
        std::string value(data.size(), '\0');
        hex(value, data);
        return value;
    }

    void unhex(std::string_view input, std::span<uint8_t> data) {
        DEBUG_ASSERT(input.size() == data.size() * 2u, "Unable to dehex string; check allocation");

        for (std::size_t i = 0; i < data.size(); ++i) {
            DEBUG_ASSERT(input[i] >= 'a' && input[i] <= 'f', "Nibble not in range");

            data[i] = ((input[i * 2] - 'a') << 4) | ((input[i * 2 + 1] - 'a') & 0xF);
        }
    }
}

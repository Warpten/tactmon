#include "libtactmon/utility/Hex.hpp"

#include <assert.hpp>

namespace libtactmon::utility {
    void hex(std::string& output, std::span<const uint8_t> data) {
        output.resize(data.size() * 2);

        for (std::size_t i = 0; i < data.size(); ++i) {
            uint8_t loPart = data[i] & 0xF;
            uint8_t hiPart = data[i] >> 4;

            static const char alphabet[] = { '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'a', 'b', 'c', 'd', 'e', 'f' };

            DEBUG_ASSERT(loPart <= 0xF, "Non hex input");
            DEBUG_ASSERT(hiPart <= 0xF, "Non hex input");

            output[i * 2] = alphabet[hiPart];
            output[i * 2 + 1] = alphabet[loPart];
        }
    }

    std::string hex(std::span<const uint8_t> data) {
        std::string value(data.size(), '\0');
        hex(value, data);
        return value;
    }

    void unhex(std::string_view input, std::span<uint8_t> data) {
        DEBUG_ASSERT(input.size() == data.size() * 2u, "Unable to dehex string; check allocation");

        static const uint_fast8_t LOOKUP[] = {
            /*  0 */ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
            /*  8 */ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
            /* 16 */ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
            /* 24 */ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
            /* 32 */ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
            /* 40 */ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
            /* 48 */ 0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
            /* 56 */ 0x08, 0x09, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
            /* 64 */ 0x00, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f, 0x00,
            /* 72 */ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
            /* 80 */ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
            /* 88 */ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
            /* 96 */ 0x00, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f
        };

        for (std::size_t i = 0; i < input.size(); i += 2) {
            DEBUG_ASSERT((input[i] >= 'a' && input[i] <= 'f') || (input[i] >= '0' && input[i] <= '9') || (input[i] >= 'A' && input[i] <= 'F'), "Nibble not in range");

            uint8_t loPart = static_cast<uint8_t>(input[i + 1]);
            uint8_t hiPart = static_cast<uint8_t>(input[i]);

            DEBUG_ASSERT(loPart <= 103, "Non hex input");
            DEBUG_ASSERT(hiPart <= 103, "Non hex input");

            data[i / 2] = LOOKUP[loPart] | (LOOKUP[hiPart] << 4);
        }
    }
}

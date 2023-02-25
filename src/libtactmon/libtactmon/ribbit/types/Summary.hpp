#pragma once

#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace libtactmon::ribbit::types {
    namespace summary {
        struct Record {
            std::string Product;
            uint32_t SequenceID = 0;
            std::string Flags;

            static std::optional<Record> Parse(std::string_view input);
        };
    }

    using Summary = std::vector<summary::Record>;
}

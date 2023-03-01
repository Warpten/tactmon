#pragma once

#include <cstdint>

#include <assert.hpp>

#include <fmt/format.h>

namespace libtactmon::ribbit {
    enum class Region : uint8_t {
        EU = 1,
        US = 2,
        KR = 3,
        CN = 4,
        TW = 5
    };

    enum class Version : uint8_t {
        V1 = 1,
        V2 = 2
    };

    enum class Command : uint8_t {
        Summary = 0,
        ProductVersions = 1,
        ProductCDNs = 2,
        ProductBGDL = 3,
        Certificate = 4,
        OCSP = 5,
    };
}

template <>
struct fmt::formatter<libtactmon::ribbit::Region> {
    template <typename ParseContext>
    constexpr auto parse(ParseContext& ctx) {
        return ctx.begin();
    }

    template <typename FormatContext>
    auto format(libtactmon::ribbit::Region region, FormatContext& ctx) {
        switch (region) {
            case libtactmon::ribbit::Region::EU: return fmt::format_to(ctx.out(), "eu");
            case libtactmon::ribbit::Region::US: return fmt::format_to(ctx.out(), "us");
            case libtactmon::ribbit::Region::KR: return fmt::format_to(ctx.out(), "kr");
            case libtactmon::ribbit::Region::CN: return fmt::format_to(ctx.out(), "cn");
            case libtactmon::ribbit::Region::TW: return fmt::format_to(ctx.out(), "tw");
        }

        DEBUG_ASSERT(false, "Unknown region type.");
        return ctx.out();
    }
};

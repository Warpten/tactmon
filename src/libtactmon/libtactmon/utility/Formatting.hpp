#pragma once

#include "libtactmon/detail/Export.hpp"
#include "libtactmon/Errors.hpp"

#include <type_traits>

#include <boost/system/error_code.hpp>

#include <fmt/format.h>

template <>
struct LIBTACTMON_API fmt::formatter<boost::system::error_code> {
    template <typename ParseContext>
    constexpr auto parse(ParseContext& ctx) {
        return ctx.begin();
    }

    template <typename FormatContext>
    auto format(boost::system::error_code const& error, FormatContext& ctx) {
        return fmt::format_to(ctx.out(), "{}", error.message());
    }
};

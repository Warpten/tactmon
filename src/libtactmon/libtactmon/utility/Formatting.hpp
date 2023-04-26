#pragma once

#include "libtactmon/detail/Export.hpp"
#include "libtactmon/Errors.hpp"

#include <type_traits>

#include <boost/system/error_code.hpp>

#include <fmt/format.h>

template <>
struct LIBTACTMON_API fmt::formatter<libtactmon::Error> {
    template <typename ParseContext>
    constexpr auto parse(ParseContext& ctx) {
        return ctx.begin();
    }

    template <typename FormatContext>
    auto format(libtactmon::Error error, FormatContext& ctx) {
        using Error = libtactmon::Error;
#define ERROR_LOGGER(ERR, LOG) case ERR: return fmt::format_to(ctx.out(), LOG)

        switch (error) {
            ERROR_LOGGER(Error::ResourceResolutionFailed, "resource resolution failed");
            ERROR_LOGGER(Error::InstallManifestNotFound, "install manifest not found");
            ERROR_LOGGER(Error::CorruptedInstallManifest, "corrupted install manifest");
            ERROR_LOGGER(Error::EncodingManifestNotFound, "encoding manifest not found");
            ERROR_LOGGER(Error::CorruptedEncodingManifest, "corrupted encoding manifest");
            ERROR_LOGGER(Error::RootManifestNotFound, "root manifest not found");
            ERROR_LOGGER(Error::CorruptedRootManifest, "corrupted root manifest");
            ERROR_LOGGER(Error::IndexNotFound, "index not found");
            ERROR_LOGGER(Error::MalformedArchive, "malformed BLTE archive");
            ERROR_LOGGER(Error::MalformedIndexFile, "malformed index file");
            ERROR_LOGGER(Error::MalformedBuildConfiguration, "malformed build configuration file");
            ERROR_LOGGER(Error::MalformedCDNConfiguration, "malformed CDN configuration file");
        }
#undef ERROR_LOGGER

        return fmt::format_to(ctx.out(), "unknwon error code #{}", static_cast<std::underlying_type_t<Error>>(error));
    }
};

template <>
struct LIBTACTMON_API fmt::formatter<boost::system::error_code> {
    template <typename ParseContext>
    constexpr auto parse(ParseContext& ctx) {
        return ctx.begin();
    }

    template <typename FormatContext>
    auto format(boost::system::error_code const& error, FormatContext& ctx) {
        return fmt::format_to(ctx.out(), "{}", error.what());
    }
};

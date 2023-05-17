#pragma once

#include "libtactmon/detail/Export.hpp"
#include "libtactmon/tact/CKey.hpp"
#include "libtactmon/tact/EKey.hpp"
#include "libtactmon/Result.hpp"

#include <cstdint>
#include <optional>
#include <string>

namespace libtactmon::io {
    struct IReadableStream;
}

namespace libtactmon::tact::config {
    /**
     * Represents a build configuration as seen on the CDN.
     * 
     * Does **not** model all properties **yet**.
     */
    struct LIBTACTMON_API BuildConfig final {
        static Result<BuildConfig> Parse(io::IReadableStream& stream);

    private:
        BuildConfig() = default;

    public:
        BuildConfig(BuildConfig&&) noexcept;
        BuildConfig(BuildConfig const&) = default;

        BuildConfig& operator = (BuildConfig&&) noexcept;
        BuildConfig& operator = (BuildConfig const&) = default;

    public:
        struct _Key {
            CKey ContentKey;
            EKey EncodingKey;
        };

        CKey Root;
        struct {
            _Key Key;
            std::size_t Size[2] = { 0, 0 };
        } Install;
        // struct { ... } Download;
        struct {
            _Key Key;
            std::size_t Size[2] = { 0, 0 };
        } Encoding;

        std::string BuildName;
        std::string BuildUID;
        std::string BuildProduct;
        std::string BuildPlaybuildInstaller;

        struct {
            struct Entry {
                std::string Name[2];
                std::size_t Size[2] = { 0, 0 };
            };

            Entry Root;
            std::vector<Entry> Entries;
        } VFS;
    };
}

#pragma once

#include <cstdint>
#include <system_error>
#include <type_traits>

namespace libtactmon {
    enum class Error {
        OK = 0, // ...

        ResourceResolutionFailed = 10,

        InstallManifestNotFound = 20,
        CorruptedInstallManifest,
        EncodingManifestNotFound,
        CorruptedEncodingManifest,
        RootManifestNotFound,
        CorruptedRootManifest,
        IndexNotFound,

        MalformedArchive,
        MalformedIndexFile,
        MalformedBuildConfiguration,
        MalformedCDNConfiguration,

        FileNotFound,
        MalformedFile,
        EncodingKeyMismatch,
        CompressionFailure,
        UnknownCompressionMode,

        BuildConfig_InvalidRoot,
        BuildConfig_InvalidInstall,
        BuildConfig_InvalidInstallSize,
        BuildConfig_InvalidEncoding,
        BuildConfig_InvalidEncodingSize,
        BuildConfig_InvalidBuildName
    };

    std::error_code make_error_code(Error err);
}

namespace std {
    template <>
    struct is_error_code_enum<libtactmon::Error> : std::true_type { };
}

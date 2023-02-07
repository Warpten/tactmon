#pragma once

#include "io/IStream.hpp"
#include "tact/CKey.hpp"
#include "tact/EKey.hpp"

#include <cstdint>
#include <string>

namespace io {
    struct IStream;
}

namespace tact::config {
    struct BuildConfig final {
        explicit BuildConfig(io::IReadableStream& fileStream);

        struct Key {
            CKey ContentKey;
            EKey EncodingKey;
        };

        CKey Root;
        struct {
            Key Key;
            size_t Size[2] = { 0, 0 };
        } Install;
        // struct { ... } Download;
        struct {
            Key Key;
            size_t Size[2] = { 0, 0 };
        } Encoding;

        std::string BuildName;
    };
}

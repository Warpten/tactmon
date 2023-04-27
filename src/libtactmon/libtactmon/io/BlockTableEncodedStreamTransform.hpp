#pragma once

#include "libtactmon/io/FileStream.hpp"
#include "libtactmon/io/MemoryStream.hpp"
#include "libtactmon/tact/CKey.hpp"
#include "libtactmon/tact/EKey.hpp"
#include "libtactmon/Errors.hpp"
#include "libtactmon/Result.hpp"

#include <cstdint>
#include <filesystem>
#include <span>

namespace libtactmon::io {
    struct BlockTableEncodedStreamTransform final {
        Result<io::GrowableMemoryStream> operator () (IReadableStream& sourceStream,
            libtactmon::tact::EKey const* encodingKey = nullptr, libtactmon::tact::CKey const* contentKey = nullptr) const noexcept;

        errors::Error operator () (IReadableStream& sourceStream, IWritableStream& targetStream,
            libtactmon::tact::EKey const* encodingKey = nullptr, libtactmon::tact::CKey const* contentKey = nullptr,
            bool validateKeys = false) const noexcept;
    };
}

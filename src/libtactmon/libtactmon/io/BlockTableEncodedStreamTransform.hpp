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
        Error operator () (IReadableStream& sourceStream, IWritableStream& targetStream, tact::EKey const* encodingKey = nullptr, tact::CKey const* contentKey = nullptr) const noexcept;

        Result<io::GrowableMemoryStream> operator () (IReadableStream& sourceStream, tact::EKey const* encodingKey = nullptr, tact::CKey const* contentKey = nullptr) const noexcept;
    };
}

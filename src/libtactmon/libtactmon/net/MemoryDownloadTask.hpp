#pragma once

#include "libtactmon/io/MemoryStream.hpp"
#include "libtactmon/net/DownloadTask.hpp"

#include <optional>

#include <boost/beast/http/dynamic_body.hpp>
#include <boost/system/error_code.hpp>

namespace libtactmon::net {
    /**
     * A download task that loads a resource to system memory.
     */
    struct MemoryDownloadTask : DownloadTask<boost::beast::http::dynamic_body, io::GrowableMemoryStream> {
        using DownloadTask::DownloadTask;

        boost::system::error_code Initialize(ValueType& body) override;
        std::optional<io::GrowableMemoryStream> TransformMessage(MessageType& body) override;
    };
}

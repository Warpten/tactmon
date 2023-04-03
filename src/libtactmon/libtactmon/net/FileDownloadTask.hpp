#pragma once

#include "libtactmon/io/FileStream.hpp"
#include "libtactmon/net/DownloadTask.hpp"

#include <cstdint>
#include <optional>
#include <string_view>

#include <boost/beast/http/file_body.hpp>
#include <boost/system/error_code.hpp>

namespace libtactmon::tact {
    struct Cache;
}

namespace libtactmon::net {
    /**
     * A download task for a file that writes the result to the disk.
     */
    struct FileDownloadTask final : DownloadTask<FileDownloadTask, boost::beast::http::file_body, io::FileStream> {
        FileDownloadTask(std::string_view resourcePath, tact::Cache& localCache) noexcept
            : DownloadTask(resourcePath), _localCache(localCache)
        { }

        FileDownloadTask(std::string_view resourcePath, std::size_t offset, std::size_t length, tact::Cache& localCache) noexcept
            : DownloadTask(resourcePath, offset, length), _localCache(localCache)
        { }

        boost::system::error_code Initialize(ValueType& body);
        std::optional<io::FileStream> TransformMessage(MessageType& body);

    private:
        tact::Cache& _localCache;
    };
}

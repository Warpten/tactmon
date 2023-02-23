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
    struct FileDownloadTask final : DownloadTask<boost::beast::http::file_body, io::FileStream> {
        FileDownloadTask(std::string_view resourcePath, tact::Cache& localCache) noexcept
            : DownloadTask(resourcePath), _localCache(localCache)
        { }

        FileDownloadTask(std::string_view resourcePath, size_t offset, size_t length, tact::Cache& localCache) noexcept
            : DownloadTask(resourcePath, offset, length), _localCache(localCache)
        { }

        boost::system::error_code Initialize(ValueType& body) override;
        std::optional<io::FileStream> TransformMessage(MessageType& body) override;

    private:
        tact::Cache& _localCache;
    };
}

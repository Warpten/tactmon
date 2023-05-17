#include "libtactmon/net/FileDownloadTask.hpp"
#include "libtactmon/tact/Cache.hpp"
#include "libtactmon/Errors.hpp"

#include <filesystem>

namespace libtactmon::net {
    boost::system::error_code FileDownloadTask::Initialize(ValueType& body) {
        std::filesystem::path absolutePath { _localCache.GetAbsolutePath(_resourcePath) };
        std::filesystem::create_directories(absolutePath.parent_path());

        boost::system::error_code ec;
        body.open(absolutePath.string().data(), boost::beast::file_mode::write, ec);
        return ec;
    }

    Result<io::FileStream> FileDownloadTask::HandleFailure(boost::beast::http::status statusCode) {
        _localCache.Delete(_resourcePath);

        return Result<io::FileStream> { errors::network::BadStatusCode(_resourcePath, "", statusCode) };
    }

    Result<io::FileStream> FileDownloadTask::TransformMessage(MessageType& message) {
        message.body().close();
        return _localCache.OpenWrite(_resourcePath);
    }
}

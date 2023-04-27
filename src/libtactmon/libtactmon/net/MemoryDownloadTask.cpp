#include "libtactmon/net/MemoryDownloadTask.hpp"

namespace libtactmon::net {
    boost::system::error_code MemoryDownloadTask::Initialize(ValueType& body) {
        return { };
    }

    Result<io::GrowableMemoryStream> MemoryDownloadTask::TransformMessage(MessageType& message) {
        return Result<io::GrowableMemoryStream> { message.body().data() };
    }

    Result<io::GrowableMemoryStream> MemoryDownloadTask::HandleFailure(boost::beast::http::status statusCode) {
        return Result<io::GrowableMemoryStream> { errors::network::BadStatusCode(_resourcePath, "", statusCode) };
    }
}

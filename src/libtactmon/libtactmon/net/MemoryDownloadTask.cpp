#include "libtactmon/net/MemoryDownloadTask.hpp"

namespace libtactmon::net {
    boost::system::error_code MemoryDownloadTask::Initialize(ValueType& body) {
        return { };
    }

    Result<io::GrowableMemoryStream> MemoryDownloadTask::TransformMessage(MessageType& message) {
        if (message.result() != boost::beast::http::status::ok)
            return Result<io::GrowableMemoryStream> { boost::beast::http::error::bad_status };

        return Result<io::GrowableMemoryStream> { message.body().data() };
    }
}

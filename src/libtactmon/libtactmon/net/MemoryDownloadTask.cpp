#include "libtactmon/net/MemoryDownloadTask.hpp"

namespace libtactmon::net {
    boost::system::error_code MemoryDownloadTask::Initialize(ValueType& body) {
        return { };
    }

    std::optional<io::GrowableMemoryStream> MemoryDownloadTask::TransformMessage(MessageType& message) {
        if (message.result() != boost::beast::http::status::ok)
            return std::nullopt;

        return io::GrowableMemoryStream { message.body().data() };
    }
}

#include "libtactmon/net/FileDownloadTask.hpp"
#include "libtactmon/tact/Cache.hpp"

#include <filesystem>

namespace libtactmon::net {
    boost::system::error_code FileDownloadTask::Initialize(ValueType& body) {
        std::filesystem::path absolutePath { _localCache.GetAbsolutePath(_resourcePath) };
        std::filesystem::create_directories(absolutePath.parent_path());

        boost::system::error_code ec;
        body.open(absolutePath.string().data(), boost::beast::file_mode::write, ec);
        return ec;
    }

    std::optional<io::FileStream> FileDownloadTask::TransformMessage(MessageType& message) {
        message.body().close();

        if (message.result() != boost::beast::http::status::ok) {
            // Ideally we would prevent beast from writing to disk if http response is not 200 OK or some
            // other 2xx code.
            _localCache.Delete(_resourcePath);
            return std::nullopt;
        }

        return _localCache.OpenWrite(_resourcePath);
    }
}

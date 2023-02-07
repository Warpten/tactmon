#include "net/DownloadTask.hpp"

namespace net {
    boost::system::error_code FileDownloadTask::Initialize(ValueType& body) {
        std::filesystem::create_directories(_pathOnDisk.parent_path());

        boost::system::error_code ec;
        body.open(_pathOnDisk.string().data(), boost::beast::file_mode::write, ec);
        return ec;
    }

    std::optional<io::FileStream> FileDownloadTask::TransformMessage(MessageType& message) {
        message.body().close();

        if (message.result() != boost::beast::http::status::ok) {
            // Ideally we would prevent beast from writing to disk if http response is not 200 OK or some
            // other 2xx code.
            std::filesystem::remove(_pathOnDisk);
            return std::nullopt;
        }

        return io::FileStream { _pathOnDisk, std::endian::little };
    }

    boost::system::error_code MemoryDownloadTask::Initialize(ValueType& body) {
        return { };
    }

    std::optional<io::mem::GrowableMemoryStream> MemoryDownloadTask::TransformMessage(MessageType& message) {
        if (message.result() != boost::beast::http::status::ok)
            return std::nullopt;

        return io::mem::GrowableMemoryStream { message.body().data(), std::endian::little };
    }
}

#include "backend/db/repository/TrackedFile.hpp"

#include <chrono>

namespace backend::db::repository {
    TrackedFile::TrackedFile(utility::ThreadPool& threadPool, pqxx::connection& connection, spdlog::async_logger& logger)
        : Base(threadPool, connection, logger)
    {
        entity::tracked_file::queries::Insert::Prepare(_connection, _logger);
        entity::tracked_file::queries::Delete::Prepare(_connection, _logger);
    }

    void TrackedFile::Insert(std::string productName, std::string filePath, std::optional<std::string> displayName) {
        std::string resolvedDisplayName = displayName.value_or(filePath);
        entity::tracked_file::queries::Insert::Execute(_connection, std::move(productName), std::move(filePath), std::move(resolvedDisplayName));
    }

    void TrackedFile::Delete(std::string productName, std::string filePath) {
        entity::tracked_file::queries::Delete::Execute(_connection, std::move(productName), std::move(filePath));
    }
}

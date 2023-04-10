#include "backend/db/repository/TrackedFile.hpp"

#include <chrono>

namespace backend::db::repository {
    TrackedFile::TrackedFile(utility::ThreadPool& threadPool, Pool& pool, spdlog::async_logger& logger)
        : Base(threadPool, pool, logger)
    {
        entity::tracked_file::queries::Insert::Prepare(pool, logger);
        entity::tracked_file::queries::Delete::Prepare(pool, logger);
    }

    void TrackedFile::Insert(std::string productName, std::string filePath, std::optional<std::string> displayName) {
        std::string resolvedDisplayName = displayName.value_or(filePath);
        entity::tracked_file::queries::Insert::Execute(_pool, std::move(productName), std::move(filePath), std::move(resolvedDisplayName));
    }

    void TrackedFile::Delete(std::string productName, std::string filePath) {
        entity::tracked_file::queries::Delete::Execute(_pool, std::move(productName), std::move(filePath));
    }
}

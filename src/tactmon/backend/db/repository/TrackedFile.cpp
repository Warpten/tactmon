#include "backend/db/repository/TrackedFile.hpp"

#include <chrono>

namespace backend::db::repository {
    TrackedFile::TrackedFile(utility::ThreadPool& threadPool, pqxx::connection& connection, spdlog::async_logger& logger)
        : Base(threadPool, connection, logger)
    {
        entity::tracked_file::queries::Insert::Prepare(_connection, _logger);
        entity::tracked_file::queries::Delete::Prepare(_connection, _logger);
    }

    void TrackedFile::Insert(std::string const& productName, std::string const& filePath) {
        Execute<entity::tracked_file::queries::Insert>(productName, filePath);
    }

    void TrackedFile::Delete(std::string const& productName, std::string const& filePath) {
        Execute<entity::tracked_file::queries::Delete>(productName, filePath);
    }
}

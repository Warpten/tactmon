#pragma once

#include "backend/db/repository/Repository.hpp"
#include "backend/db/entity/TrackedFile.hpp"
#include "utility/ThreadPool.hpp"

#include <optional>
#include <string>
#include <vector>

#include <pqxx/connection>

#include <spdlog/async_logger.h>

namespace backend::db::repository {
    struct TrackedFile : Repository<entity::tracked_file::Entity, entity::tracked_file::queries::Select, entity::tracked_file::id, true> {
        using Base = Repository<entity::tracked_file::Entity, entity::tracked_file::queries::Select, entity::tracked_file::id, true>;

        TrackedFile(utility::ThreadPool& threadPool, pqxx::connection& connection, spdlog::async_logger& logger);

        /**
         * Registers a file for tracking.
         *
         * @param[in] productName The product for which this file should be tracked.
         * @param[in] filePath    The path to the file to track.
         * @param[in] displayName A name to use to display this file when generating buttons for build pushes.
         */
        void Insert(std::string productName, std::string filePath, std::optional<std::string> displayName = std::nullopt);

        /**
         * Unregisters a file for tracking.
         * 
         * @param[in] productName The product for which this file should be tracked.
         * @param[in] filePath    The path to the file to track.
         */
        void Delete(std::string productName, std::string filePath);
    };
}

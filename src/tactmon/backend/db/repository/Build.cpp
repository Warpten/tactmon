#include "backend/db/repository/Build.hpp"

#include <chrono>

namespace backend::db::repository {
    Build::Build(utility::ThreadPool& threadPool, pqxx::connection& connection, spdlog::async_logger& logger)
        : Base(threadPool, connection, logger)
    {
        entity::build::queries::SelById::Prepare(_connection, logger);
        entity::build::queries::SelByName::Prepare(_connection, logger);
        entity::build::queries::SelByProduct::Prepare(_connection, logger);
        entity::build::queries::SelStatistics::Prepare(_connection, logger);

        entity::build::queries::Insert::Prepare(_connection, logger);
    }

    std::optional<entity::build::Entity> Build::GetByBuildName(std::string buildName, std::string region) const {
        return entity::build::queries::SelByName::ExecuteOne(_connection, std::move(buildName), std::move(region));
    }

    std::vector<entity::build::dto::BuildName> Build::GetByProductName(std::string productName) const {
        return entity::build::queries::SelByProduct::Execute(_connection, std::move(productName));
    }

    std::optional<entity::build::dto::BuildStatistics> Build::GetStatisticsForProduct(std::string productName) const {
        return entity::build::queries::SelStatistics::ExecuteOne(_connection, std::move(productName));
    }

    std::optional<entity::build::Entity> Build::Insert(std::string region, std::string productName,
        std::string buildName, std::string buildConfig, std::string cdnConfig)
    {
        using namespace std::chrono;
        using clock = system_clock;

        return entity::build::queries::Insert::ExecuteOne(_connection, std::move(region), std::move(productName),
            std::move(buildName), std::move(buildConfig), std::move(cdnConfig),
            static_cast<uint64_t>(duration_cast<seconds>(clock::now().time_since_epoch()).count())
        );
    }
}

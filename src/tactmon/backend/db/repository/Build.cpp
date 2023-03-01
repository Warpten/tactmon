#include "backend/db/repository/Build.hpp"

#include <chrono>

namespace backend::db::repository {
    Build::Build(utility::ThreadPool& threadPool, pqxx::connection& connection, spdlog::async_logger& logger)
        : Base(threadPool, connection, logger)
    {
        entity::build::queries::SelById::Prepare(_connection, _logger);
        entity::build::queries::SelByName::Prepare(_connection, _logger);
        entity::build::queries::SelByProduct::Prepare(_connection, _logger);
        entity::build::queries::SelStatistics::Prepare(_connection, _logger);

        entity::build::queries::Insert::Prepare(_connection, _logger);
    }

    std::optional<entity::build::Entity> Build::GetByBuildName(std::string const& buildName, std::string const& region) const {
        return ExecuteOne<entity::build::queries::SelByName>(buildName, region);
    }

    std::vector<entity::build::dto::BuildName> Build::GetByProductName(std::string const& productName) const {
        return Execute<entity::build::queries::SelByProduct>(productName);
    }

    std::optional<entity::build::dto::BuildStatistics> Build::GetStatisticsForProduct(std::string const& productName) const {
        return ExecuteOne<entity::build::queries::SelStatistics>(productName);
    }

    void Build::Insert(std::string const& region, std::string const& productName, std::string const& buildName, std::string const& buildConfig, std::string const& cdnConfig) {
        using namespace std::chrono;
        using clock = system_clock;

        ExecuteOne<entity::build::queries::Insert>(region, productName, buildName, buildConfig, cdnConfig,
            static_cast<uint64_t>(duration_cast<seconds>(clock::now().time_since_epoch()).count())
        );
    }
}

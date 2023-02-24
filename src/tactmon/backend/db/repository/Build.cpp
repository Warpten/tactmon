#include "backend/db/repository/Build.hpp"

namespace backend::db::repository {
    Build::Build(boost::asio::io_context::strand context, pqxx::connection& connection, spdlog::async_logger& logger)
        : Base(context, connection, logger)
    {
        entity::build::queries::SelById::Prepare(_connection, _logger);
        entity::build::queries::SelByName::Prepare(_connection, _logger);
        entity::build::queries::SelByProduct::Prepare(_connection, _logger);
        entity::build::queries::SelStatistics::Prepare(_connection, _logger);
    }

    std::optional<entity::build::Entity> Build::GetByBuildName(std::string const& buildName) const {
        return ExecuteOne<entity::build::queries::SelByName>(buildName);
    }

    std::vector<entity::build::dto::BuildName> Build::GetByProductName(std::string const& productName) const {
        return Execute<entity::build::queries::SelByProduct>(productName);
    }

    std::optional<entity::build::dto::BuildStatistics> Build::GetStatisticsForProduct(std::string const& productName) const {
        return ExecuteOne<entity::build::queries::SelStatistics>(productName);
    }
}

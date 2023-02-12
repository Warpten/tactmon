#include "backend/db/repository/Build.hpp"

namespace backend::db::repository {
    Build::Build(std::shared_ptr<boost::asio::io_context> context, std::shared_ptr<pqxx::connection> connection)
        : Base(context, connection)
    {
        entity::build::queries::SelById::Prepare(*_connection, _logger);
        entity::build::queries::SelByName::Prepare(*_connection, _logger);
        entity::build::queries::SelByProduct::Prepare(*_connection, _logger);
        entity::build::queries::SelStatistics::Prepare(*_connection, _logger);
    }

    std::optional<entity::build::Entity::as_projection> Build::GetByBuildName(std::string const& buildName) const {
        return ExecuteOne<entity::build::queries::SelByName>(buildName);
    }

    std::vector<entity::build::dto::BuildName> Build::GetByProductName(std::string const& productName) const {
        return Execute<entity::build::queries::SelByProduct>(productName);
    }

    std::optional<entity::build::dto::BuildStatistics> Build::GetStatisticsForProduct(std::string const& productName) const {
        return ExecuteOne<entity::build::queries::SelStatistics>(productName);
    }
}

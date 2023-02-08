#include "backend/Database.hpp"

#include <ext/Literal.hpp>

#include <format>
#include <optional>
#include <sstream>
#include <tuple>
#include <type_traits>
#include <vector>

#include <pqxx/pqxx>

namespace backend {
    Database::Database(std::string_view username, std::string_view password, std::string_view host, uint64_t port, std::string_view name)
        : _connection(std::format("user={} password={} host={} port={} dbname={} target_session_attrs=read-write", username, password, host, port, name))
    {
        entities::build::queries::SelectByName::Prepare(_connection);
        entities::build::queries::SelectForProduct::Prepare(_connection);
    }

    auto Database::SelectBuild(std::string const& buildName) -> std::optional<entities::build::Entity> {
        pqxx::transaction<pqxx::read_committed, pqxx::write_policy::read_only> transaction { _connection };
        return entities::build::queries::SelectByName::ExecuteOne(transaction, std::tuple { buildName });
    }

    auto Database::SelectBuilds(std::string const& productName) -> std::vector<entities::build::dto::BuildName> {
        pqxx::transaction<pqxx::read_committed, pqxx::write_policy::read_only> transaction { _connection };
        return entities::build::queries::SelectForProduct::Execute(transaction, productName);
    }
}

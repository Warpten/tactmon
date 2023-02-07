#include "backend/Database.hpp"

#include <ext/Literal.hpp>

#include <format>

#include <pqxx/pqxx>

namespace backend {
    template <ext::Literal Identifier, ext::Literal Query, typename... Args>
    struct NamedQuery {
        static void Prepare(pqxx::connection& connection) {
            connection.prepare(pqxx::zview{ Identifier.Value, Identifier.Size }, pqxx::zview{ Query.Value, Query.Size });
        }

        template <pqxx::isolation_level ISOLATION, pqxx::write_policy POLICY>
        static pqxx::result Execute(pqxx::transaction<ISOLATION, POLICY>& transaction, Args... args) {
            return transaction.exec_prepared(pqxx::zview{ Identifier.Value, Identifier.Size }, std::forward<Args>(args)...);
        }

        template <pqxx::isolation_level ISOLATION, pqxx::write_policy POLICY>
        static pqxx::row ExecuteOne(pqxx::transaction<ISOLATION, POLICY>& transaction, Args... args) {
            return transaction.exec_prepared1(pqxx::zview{ Identifier.Value, Identifier.Size }, std::forward<Args>(args)...);
        }
    };

#define MAKE_PREPARED_STMT(NAME, QUERY, ...) using NAME = NamedQuery<#NAME, QUERY, __VA_ARGS__>
    MAKE_PREPARED_STMT(SelectBuildByName, "SELECT id, build_name, build_config, cdn_config FROM public.builds WHERE build_name = $1 LIMIT 1", std::string);
    MAKE_PREPARED_STMT(SelectBuildsForProduct, "SELECT id, build_name, build_config, cdn_config FROM public.builds WHERE product_name = $1", std::string);
#undef MAKE_PREPARED_STMT

    Database::Database(std::string_view username, std::string_view password, std::string_view host, uint64_t port, std::string_view name)
        : _connection(std::format("user={} password={} host={} port={} dbname={} target_session_attrs=read-write", username, password, host, port, name))
    {
        SelectBuildByName::Prepare(_connection);
        SelectBuildsForProduct::Prepare(_connection);
    }

    auto Database::SelectBuild(std::string const& buildName) -> std::optional<Build> {
        pqxx::transaction<pqxx::read_committed, pqxx::write_policy::read_only> transaction { _connection };
        pqxx::row resultSet = SelectBuildByName::ExecuteOne(transaction, buildName);

        return Build::TryParse(resultSet);;
    }

    auto Database::SelectBuilds(std::string const& productName) -> std::vector<Build> {
        pqxx::transaction<pqxx::read_committed, pqxx::write_policy::read_only> transaction{ _connection };
        pqxx::result resultSet = SelectBuildsForProduct::Execute(transaction, productName);

        std::vector<Build> builds;
        for (pqxx::row row : resultSet)
            if (auto record = Build::TryParse(row); record.has_value())
                builds.emplace_back(*record);

        return builds;
    }

    /* static */ auto Database::Build::TryParse(pqxx::row const& row) -> std::optional<Build> {
        if (row.empty())
            return std::nullopt;

        return Build {
            .ID = row["id"].as<uint32_t>(0),
            .Name = row["build_name"].as<std::string>(),
            .BuildConfig = row["build_config"].as<std::string>(),
            .CdnConfig = row["cdn_config"].as<std::string>(),
        };
    }
}

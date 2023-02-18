#pragma once

#include <optional>
#include <tuple>
#include <vector>

#include <ext/Literal.hpp>

#include <pqxx/connection>
#include <pqxx/result>
#include <pqxx/row>
#include <pqxx/transaction_base>

#include <spdlog/async_logger.h>
#include <spdlog/logger.h>

namespace backend::db {
    template <ext::Literal ALIAS, typename QUERY>
    struct PreparedStatement final {
        using entity_type = typename QUERY::entity_type;
        using projection_type = typename QUERY::projection_type;
        using transaction_type = typename QUERY::transaction_type;

        static void Prepare(pqxx::connection& connection) {
            std::string rendererQuery = QUERY::Render();
            connection.prepare(pqxx::zview{ ALIAS.Value, ALIAS.Size - 1 }, rendererQuery);
        }

        static void Prepare(pqxx::connection& connection, std::shared_ptr<spdlog::async_logger> logger) {
            std::string rendererQuery = QUERY::Render();
            if (logger != nullptr)
                logger->debug("Preparing query {}: '{}'", ALIAS.Value, rendererQuery);

            connection.prepare(pqxx::zview{ ALIAS.Value, ALIAS.Size - 1 }, rendererQuery);
        }

        static void Prepare(pqxx::connection& connection, std::shared_ptr<spdlog::logger> logger) {
            std::string rendererQuery = QUERY::Render();
            if (logger != nullptr)
                logger->debug("Preparing query {}: '{}'", ALIAS.Value, rendererQuery);

            connection.prepare(pqxx::zview{ ALIAS.Value, ALIAS.Size - 1 }, rendererQuery);
        }

        template <typename... Args>
        static auto ExecuteOne(pqxx::transaction_base& transaction, std::tuple<Args...> args)
            -> std::optional<typename QUERY::projection_type>
        {
            pqxx::result resultSet = Execute_(transaction, args);
            if (resultSet.size() != 1)
                return std::nullopt;

            pqxx::row record = resultSet[0];
            return typename QUERY::projection_type { record };
        }

        template <typename... Args>
        static auto Execute(pqxx::transaction_base& transaction, std::tuple<Args...> args)
            -> std::vector<typename QUERY::projection_type>
        {
            pqxx::result resultSet = Execute_(transaction, args);

            std::vector<typename QUERY::projection_type> entities;
            entities.reserve(resultSet.size());
            for (pqxx::row row : resultSet)
                entities.emplace_back(row);
            return entities;
        }

    private:

        template <typename... Args>
        static auto Execute_(pqxx::transaction_base& transaction, std::tuple<Args...> args) {
            return std::apply([&](auto... arg) mutable {
                return transaction.exec_prepared(pqxx::zview { ALIAS.Value, ALIAS.Size - 1 }, arg...);
            }, args);
        }

        template <typename... Args>
        static auto ExecuteOne_(pqxx::transaction_base& transaction, std::tuple<Args...> args) {
            return std::apply([&](auto... arg) mutable {
                return transaction.exec_prepared1(pqxx::zview { ALIAS.Value, ALIAS.Size - 1 }, arg...);
            }, args);
        }
    };
}

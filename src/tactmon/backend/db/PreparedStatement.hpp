#pragma once

#include "backend/db/Queries.hpp"
#include "utility/Literal.hpp"

#include <optional>
#include <tuple>
#include <vector>

#include <pqxx/connection>
#include <pqxx/result>
#include <pqxx/row>
#include <pqxx/transaction_base>

#include <spdlog/async_logger.h>
#include <spdlog/logger.h>

namespace backend::db {
    namespace detail {
        template <typename T>
        concept IsLogger = requires (T instance) {
            { instance.error("") } -> std::same_as<void>;
            { instance.info("") } -> std::same_as<void>;
            { instance.debug("") } -> std::same_as<void>;
        };
    }

    template <utility::Literal ALIAS, typename QUERY>
    struct PreparedStatement final {
        using entity_type = typename QUERY::entity_type;
        using projection_type = typename QUERY::projection_type;
        using transaction_type = typename QUERY::transaction_type;

        static void Prepare(pqxx::connection& connection) {
            std::string rendererQuery = QUERY::Render();
            connection.prepare(pqxx::zview{ ALIAS.Value, ALIAS.Size - 1 }, rendererQuery);
        }

        template <detail::IsLogger Logger>
        static void Prepare(pqxx::connection& connection, Logger& logger) {
            std::string rendererQuery = QUERY::Render();
            logger.debug("Preparing query {}: '{}'", ALIAS.Value, rendererQuery);

            connection.prepare(pqxx::zview{ ALIAS.Value, ALIAS.Size - 1 }, rendererQuery);
        }

        template <detail::IsLogger Logger, typename... Args>
        static auto ExecuteOne(pqxx::transaction_base& transaction, Logger& logger, std::tuple<Args...> args)
            -> std::optional<typename QUERY::projection_type>
        {
            pqxx::result resultSet = Execute_(transaction, logger, args);
            if (resultSet.size() != 1)
                return std::nullopt;

            pqxx::row record = resultSet[0];
            return typename QUERY::projection_type { record };
        }

        template <detail::IsLogger Logger, typename... Args>
        static auto Execute(pqxx::transaction_base& transaction, Logger& logger, std::tuple<Args...> args)
            -> std::vector<typename QUERY::projection_type>
        {
            pqxx::result resultSet = Execute_(transaction, logger, args);

            std::vector<typename QUERY::projection_type> entities;
            entities.reserve(resultSet.size());
            for (pqxx::row row : resultSet)
                entities.emplace_back(row);
            return entities;
        }

    private:
        template <typename Logger, typename... Args>
        static auto Execute_(pqxx::transaction_base& transaction, Logger& logger, std::tuple<Args...> args) {
            return std::apply(
                [&](auto... arg) mutable {
                    try {
                        auto result = transaction.exec_prepared(pqxx::zview{ ALIAS.Value, ALIAS.Size - 1 }, arg...);
                        transaction.commit();
                        return result;
                    } catch (std::exception const& ex) {
                        transaction.abort();

                        logger.error(ex.what());

                        return pqxx::result { };
                    }
                }, args);
        }

        template <typename Logger, typename... Args>
        static auto ExecuteOne_(pqxx::transaction_base& transaction, Logger& logger, std::tuple<Args...> args) {
            return std::apply(
                [&](auto... arg) mutable {
                    try {
                    auto result = transaction.exec_prepared1(pqxx::zview{ ALIAS.Value, ALIAS.Size - 1 }, arg...);
                    transaction.commit();
                    return result;
                } catch (std::exception const& ex) {
                    transaction.abort();

                    logger.error(ex.what());

                    return pqxx::row { };
                }
            }, args);
        }
    };
}

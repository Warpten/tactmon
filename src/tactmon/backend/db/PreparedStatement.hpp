#pragma once

#include <optional>
#include <tuple>
#include <vector>

#include <ext/Literal.hpp>

#include <pqxx/connection>
#include <pqxx/result>
#include <pqxx/row>
#include <pqxx/transaction_base>

namespace backend::db {
    template <ext::Literal ALIAS, typename QUERY>
    struct PreparedStatement final {
        using entity_type = typename QUERY::entity_type;
        using projection_type = typename QUERY::projection_type;
        using transaction_type = typename QUERY::transaction_type;

        static void Prepare(pqxx::connection& connection)
        {
            connection.prepare(pqxx::zview{ ALIAS.Value, ALIAS.Size - 1 }, QUERY::Render());
        }

        template <typename... Args>
        static auto ExecuteOne(pqxx::transaction_base& transaction, std::tuple<Args...> args)
            -> std::optional<typename QUERY::projection_type>
        {
            pqxx::row resultSet = ExecuteOne_(transaction, args);
            if (resultSet.empty())
                return std::nullopt;

            return typename QUERY::projection_type{ resultSet };
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

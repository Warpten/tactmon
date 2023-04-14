#pragma once

#include "backend/ConnectionPool.hpp"
#include "utility/Literal.hpp"
#include "utility/Tuple.hpp"

#include <memory>
#include <stdexcept>
#include <type_traits>
#include <utility>
#include <vector>

#include <pqxx/connection>
#include <pqxx/result>

#include <spdlog/async_logger.h>

namespace backend::db {
    namespace detail {
        template <utility::Literal, typename...> struct PreparedStatementBase;

        template <utility::Literal NAME, typename TRANSACTION, typename RESULT, typename... Ts>
        struct PreparedStatementBase<NAME, TRANSACTION, RESULT, utility::tuple<Ts...>> {
            using work_type = TRANSACTION;
            using result_type = RESULT;

            PreparedStatementBase() = delete;

            PreparedStatementBase(PreparedStatementBase const&) = delete;
            PreparedStatementBase(PreparedStatementBase&&) noexcept = delete;

            PreparedStatementBase& operator = (PreparedStatementBase const&) = delete;
            PreparedStatementBase&& operator = (PreparedStatementBase&&) noexcept = delete;

        public:
            /**
             * Executes this statement with the provided parameters.
             *
             * @param pool       A connection pool from which a connection will be rented from to execute this statement.
             * @param ...args    Bound parameters for the query.
             *
             * @returns A single record if exactly one is returned by the query; an empty optional otherwise.
             */
            static auto ExecuteOne(Pool& pool, Ts... args) {
                Connection connection = pool.Open();
                work_type work{ connection.connection() };

                return ExecuteOne(work, std::forward<Ts>(args)...);
            }

            /**
             * Executes this statement with the provided parameters.
             *
             * @param pool    A connection pool from which a connection will be rented from to execute this statement.
             * @param ...args Bound parameters for the query.
             * 
             * @returns A collection of elements as read from the database.
             */
            static auto Execute(Pool& pool, Ts... args) {
                Connection connection = pool.Open();
                work_type work { connection.connection() };

                return Execute(work, std::forward<Ts>(args)...);
            }

            /**
             * Executes this statement with the provided parameters.
             *
             * @param work    A transactional unit of work within which the query will be executed.
             * @param ...args Bound parameters for the query.
             * 
             * @returns A single record if exactly one is returned by the query; an empty optional otherwise.
             */
            template <typename U = result_type>
            static auto ExecuteOne(work_type& work, Ts... args) {
                try {
                    pqxx::result resultSet = work.exec_prepared(NAME.Value, std::forward<Ts&&>(args)...);
                    work.commit();

                    if constexpr (!std::is_void_v<U>) {
                        if (resultSet.size() != 1)
                            return std::optional<U> { std::nullopt };

                        pqxx::row resultRow = resultSet[0];
                        return std::optional<U> { resultRow };
                    }
                } catch (const std::exception& ex) {
                    work.abort();

                    if constexpr (!std::is_void_v<U>)
                        return std::optional<U> { std::nullopt };
                }
            }

            /**
             * Executes this statement with the provided parameters.
             *
             * @param work    A transactional unit of work within which the query will be executed.
             * @param ...args Bound parameters for the query.
             * 
             * @returns A collection of elements as read from the database.
             */
            template <typename U = result_type>
            static auto Execute(work_type& work, Ts... args) {
                try {
                    pqxx::result resultSet = work.exec_prepared(NAME.Value, std::forward<Ts>(args)...);
                    work.commit();

                    if constexpr (!std::is_void_v<U>) {
                        std::vector<U> entities;
                        entities.reserve(resultSet.size());

                        for (pqxx::row row : resultSet)
                            entities.emplace_back(row);

                        return entities;
                    }
                } catch (const std::exception& ex) {
                    work.abort();

                    if constexpr (!std::is_void_v<U>) {
                        return std::vector<U>{};
                    }
                }
            }
        };
    }

    /**
     * A prepared statement.
     *
     * @tparam NAME  The prepared statement's name.
     * @tparam QUERY The query being prepared.
     */
    template <utility::Literal NAME, typename QUERY>
    struct PreparedStatement final : private detail::PreparedStatementBase<NAME,
        typename QUERY::transaction_type, typename QUERY::result_type, typename QUERY::parameter_types>
    {
        static void Prepare(Pool& pool) {
            std::string queryString = QUERY::render();

            pool.Prepare(std::string_view { NAME.Value, NAME.Size - 1 }, queryString);
        }

        static void Prepare(Pool& pool, spdlog::async_logger& logger) {
            std::string rendererQuery = QUERY::render();
            logger.debug("Preparing query {}: '{}'", NAME.Value, rendererQuery);

            pool.Prepare(std::string_view { NAME.Value, NAME.Size - 1 }, rendererQuery);
        }

    private:
        using Base = detail::PreparedStatementBase<NAME,
            typename QUERY::transaction_type, typename QUERY::result_type, typename QUERY::parameter_types
        >;

    public:
        using Base::Execute;
        using Base::ExecuteOne;
    };
}

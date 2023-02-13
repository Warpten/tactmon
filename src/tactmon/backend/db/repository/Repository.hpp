#pragma once

#include "backend/db/DSL.hpp"

#include <chrono>
#include <format>
#include <functional>
#include <memory>
#include <mutex>
#include <optional>

#include <boost/asio/placeholders.hpp>
#include <boost/asio/high_resolution_timer.hpp>
#include <boost/container/flat_map.hpp>
#include <boost/range/adaptor/map.hpp>

#include <pqxx/connection>

#include <logging/Sinks.hpp>
#include <spdlog/stopwatch.h>
#include <fmt/chrono.h>

namespace backend::db::repository {
    using namespace std::chrono_literals;

    template <typename ENTITY, typename LOAD_STATEMENT, typename PRIMARY_KEY, bool CACHING = false>
    // requires std::is_same_v<ENTITY, typename LOAD_STATEMENT::entity_type>
    struct Repository {
    protected:
        template <auto B = CACHING> // Make dependant
        explicit Repository(std::shared_ptr<boost::asio::io_context> context, std::shared_ptr<pqxx::connection> connection,
            std::chrono::seconds interval = 60s, std::enable_if_t<B, int> _ = 0)
            : _logger(logging::GetAsyncLogger(std::format("db::{}", ENTITY::Name.Value))),
                _connection(connection), _context(context), _refreshTimer(*_context), _refreshInterval(interval)
        {
            LOAD_STATEMENT::Prepare(*_connection, _logger);

            Refresh_({ });
        }

        template <auto B = CACHING> // Make dependant
        explicit Repository(std::shared_ptr<pqxx::connection> connection, std::enable_if_t<!B, int> _ = 0)
            : _logger(logging::GetAsyncLogger(std::format("db::{}", ENTITY::Name.Value))),
                _connection(connection)
        {
            LOAD_STATEMENT::Prepare(*_connection, _logger);

            LoadFromDB_();
        }

        explicit Repository(Repository&& other) noexcept : _connection(other._connection),
            _context(other._context),
            _refreshTimer(std::move(other._refreshTimer)),
            _refreshInterval(other._refreshInterval),
            _storage(std::move(other._storage))
        {
            if (other._refreshTimer.has_value())
                other._refreshTimer->cancel();
        }

        Repository& operator = (Repository&& other) noexcept {
            _context = other._context;
            _connection = other._connection;
            _refreshTimer = std::move(other._refreshTimer);
            _refreshInterval = other._refreshInterval;
            _storageMutex = std::move(other._storageMutex);
            _storage = std::move(other._storage);

            if (other._refreshTimer.has_value())
                other._refreshTimer->cancel();
        }

    public:
        std::optional<typename ENTITY::as_projection> operator [] (typename PRIMARY_KEY::value_type id) const {
            auto accessor = [&]() -> std::optional<typename ENTITY::as_projection> {
                auto itr = _storage.find(id);
                if (itr == _storage.end())
                    return std::nullopt;

                return itr->second;
            };

            if constexpr (CACHING) {
                std::unique_lock<std::mutex> lock(_storageMutex);
                return accessor();
            } else {
                return accessor();
            }
        }

        /**
         * Executes a given prepared statement.
         */
        template <typename PREPARED_STATEMENT, typename... Args>
        auto ExecuteOne(Args&&... args) const
            -> std::optional<typename PREPARED_STATEMENT::projection_type> 
        {
            using transaction_type = typename PREPARED_STATEMENT::transaction_type;

            transaction_type transaction { *_connection };
            return PREPARED_STATEMENT::ExecuteOne(transaction, std::forward_as_tuple<Args...>(args...));
        }

        /**
         * Executes a given prepared statement
         */
        template <typename PREPARED_STATEMENT, typename... Args>
        auto Execute(Args&&... args) const
            -> std::vector<typename PREPARED_STATEMENT::projection_type>
        {
            using transaction_type = typename PREPARED_STATEMENT::transaction_type;

            transaction_type transaction { *_connection };
            return PREPARED_STATEMENT::Execute(transaction, std::forward_as_tuple<Args...>(args...));
        }

        auto values() const {
            return boost::adaptors::values(_storage);
        }

    private:
        /**
         * Reloads cached data from the database. This function automatically executes every minute.
         */
        void Refresh_(boost::system::error_code const& ec) {
            if (ec == boost::asio::error::operation_aborted)
                return;

            LoadFromDB_();

            _refreshTimer->expires_at(std::chrono::high_resolution_clock::now() + _refreshInterval);
            _refreshTimer->async_wait([this](boost::system::error_code const& ec) {
                this->Refresh_(ec);
            });
        }

        void LoadFromDB_() {
            namespace chrono = std::chrono;

            _logger->info("Refreshing cache entries for db::{}.", ENTITY::Name.Value);
            spdlog::stopwatch sw { };

            std::vector<typename ENTITY::as_projection> storage = Execute<LOAD_STATEMENT>();

            _logger->debug("Loaded {} db::{} entries from database in {}.",
                storage.size(), ENTITY::Name.Value, chrono::duration_cast<chrono::milliseconds>(sw.elapsed()));

            sw.reset();

            std::unique_lock<std::mutex> lock(_storageMutex);
            _storage.clear();
            for (auto&& entity : storage)
                _storage.emplace(db::get<PRIMARY_KEY>(entity), entity);

            _logger->debug("Cache for db::{} filled in {}.",
                ENTITY::Name.Value, chrono::duration_cast<chrono::milliseconds>(sw.elapsed()));
        }

    protected:
        std::shared_ptr<pqxx::connection> _connection;
        std::shared_ptr<spdlog::async_logger> _logger;

    private:

        std::shared_ptr<boost::asio::io_context> _context = nullptr;
        std::optional<boost::asio::high_resolution_timer> _refreshTimer = std::nullopt;
        std::chrono::seconds _refreshInterval;
        mutable std::mutex _storageMutex;

        boost::container::flat_map<typename PRIMARY_KEY::value_type, typename ENTITY::as_projection> _storage;
    };
}

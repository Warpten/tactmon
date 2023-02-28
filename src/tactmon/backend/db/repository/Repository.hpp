#pragma once

#include "backend/db/DSL.hpp"
#include "utility/ThreadPool.hpp"

#include <chrono>
#include <functional>
#include <memory>
#include <mutex>
#include <optional>

#include <boost/asio/bind_executor.hpp>
#include <boost/asio/high_resolution_timer.hpp>
#include <boost/asio/io_context.hpp>
#include <boost/asio/io_context_strand.hpp>
#include <boost/asio/placeholders.hpp>
#include <boost/container/flat_map.hpp>
#include <boost/range/adaptor/map.hpp>

#include <fmt/chrono.h>
#include <fmt/format.h>

#include <pqxx/connection>

#include <spdlog/async_logger.h>
#include <spdlog/stopwatch.h>

namespace backend::db::repository {
    using namespace std::chrono_literals;

    namespace detail {
        namespace asio = boost::asio;

        template <bool CACHING> struct repository_base;

        template <> struct repository_base<true> {
            repository_base(utility::ThreadPool& threadPool, std::chrono::seconds refreshInterval)
                : _threadPool(threadPool), _refreshTimer(threadPool.executor()), _refreshInterval(refreshInterval)
            { }

            // io_context::strand is immovable?
            repository_base(repository_base&&) noexcept = delete;
            repository_base& operator = (repository_base&&) noexcept = delete;

            template <typename T>
            void Refresh_(boost::system::error_code const& ec, T* self) {
                if (ec == boost::asio::error::operation_aborted)
                    return;

                self->LoadFromDB_();

                _refreshTimer.expires_at(std::chrono::high_resolution_clock::now() + _refreshInterval);
                _refreshTimer.async_wait([this, self](boost::system::error_code const& ec) {
                    this->Refresh_(ec, self);
                });
            }

            template <typename Callback>
            decltype(auto) WithMutex_(Callback callback) const {
                std::unique_lock<std::mutex> lock(_storageMutex);
                return callback();
            }

            utility::ThreadPool& _threadPool;
            boost::asio::high_resolution_timer _refreshTimer;
            std::chrono::seconds _refreshInterval;
            mutable std::mutex _storageMutex;
        };

        template <> struct repository_base<false> {
            repository_base() = default;

            template <typename T> void Refresh_(boost::system::error_code const& ec) { }

            template <typename Callback> decltype(auto) WithMutex_(Callback callback) const { return callback(); }
        };
    }

    template <typename ENTITY, typename LOAD_STATEMENT, typename PRIMARY_KEY, bool CACHING = false>
    // requires std::is_same_v<ENTITY, typename LOAD_STATEMENT::entity_type>
    struct Repository : private detail::repository_base<CACHING> {
        template <bool> friend struct detail::repository_base;

        using repository_base = detail::repository_base<CACHING>;

    protected:
        template <auto B = CACHING, std::enable_if_t<B, int> _ = 0> // Make dependant
        explicit Repository(utility::ThreadPool& threadPool, pqxx::connection& connection, spdlog::async_logger& logger, std::chrono::seconds interval = 60s)
            : repository_base(threadPool, interval), _logger(logger),
                _connection(connection)
        {
            LOAD_STATEMENT::Prepare(_connection, _logger);

            repository_base::Refresh_({ }, this);
        }

        template <auto B = CACHING, std::enable_if_t<!B, int> _ = 0>
        Repository(pqxx::connection& connection, spdlog::async_logger& logger)
            : repository_base(), _logger(logger),
                _connection(connection)
        {
            LOAD_STATEMENT::Prepare(_connection, _logger);

            LoadFromDB_();
        }

        explicit Repository(Repository&& other) noexcept = delete;
        Repository& operator = (Repository&& other) noexcept = delete;

    public:
        std::optional<ENTITY> operator [] (typename PRIMARY_KEY::value_type id) const {
            return repository_base::WithMutex_([&]() -> std::optional<ENTITY> {
                auto itr = _storage.find(id);
                if (itr == _storage.end())
                    return std::nullopt;

                return itr->second;
            });
        }

        size_t size() const { return _storage.size(); }
        auto values() const { return boost::adaptors::values(_storage); }

        /**
         * Executes a given prepared statement.
         */
        template <typename PREPARED_STATEMENT, typename... Args>
        auto ExecuteOne(Args... args) const
            -> std::optional<typename PREPARED_STATEMENT::projection_type> 
        {
            using transaction_type = typename PREPARED_STATEMENT::transaction_type;

            transaction_type transaction { _connection };
            return PREPARED_STATEMENT::ExecuteOne(transaction, _logger, std::tuple<Args...> { args... });
        }

        /**
         * Executes a given prepared statement
         */
        template <typename PREPARED_STATEMENT, typename... Args>
        auto Execute(Args... args) const
            -> std::vector<typename PREPARED_STATEMENT::projection_type>
        {
            using transaction_type = typename PREPARED_STATEMENT::transaction_type;

            transaction_type transaction { _connection };
            return PREPARED_STATEMENT::Execute(transaction, _logger, std::tuple<Args...> { args... });
        }

        template <typename Callback>
        void WithValues(Callback&& callback) const {
            repository_base::WithMutex_([&]() {
                callback(boost::adaptors::values(_storage));
            });
        }

    private:
        /**
         * Reloads cached data from the database. This function automatically executes every minute.
         */
        void LoadFromDB_() {
            namespace chrono = std::chrono;

            _logger.info("Refreshing cache entries for db::{}.", ENTITY::Name.Value);

            spdlog::stopwatch sw { };

            std::vector<ENTITY> storage = Execute<LOAD_STATEMENT>();

            _logger.debug("Loaded {} db::{} entries from database in {}.",
                storage.size(), ENTITY::Name.Value, chrono::duration_cast<chrono::milliseconds>(sw.elapsed()));

            sw.reset();

            repository_base::WithMutex_([this, &storage]() {
                this->_storage.clear();
                for (auto&& entity : storage)
                    this->_storage.emplace(db::get<PRIMARY_KEY>(entity), entity);
            });

            _logger.debug("Cache for db::{} filled in {}.",
                ENTITY::Name.Value, chrono::duration_cast<chrono::milliseconds>(sw.elapsed()));
        }

    protected:
        spdlog::async_logger& _logger;
        pqxx::connection& _connection;
        
    private:
        boost::container::flat_map<typename PRIMARY_KEY::value_type, ENTITY> _storage;
    };
}

#pragma once

#include "backend/db/orm/PreparedStatement.hpp"
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

        template <typename, typename, typename, bool> struct repository_base;

        template <typename LOAD_STATEMENT, typename PK, typename ENTITY>
        struct repository_base<LOAD_STATEMENT, PK, ENTITY, true> {
            repository_base(utility::ThreadPool& threadPool, pqxx::connection& connection, std::chrono::seconds refreshInterval, spdlog::async_logger& logger)
                : _refreshTimer(threadPool.pool_executor()), _refreshInterval(refreshInterval), _logger(logger), _connection(connection)
            {
                LOAD_STATEMENT::Prepare(_connection, _logger);
            }

            // io_context::strand is immovable?
            repository_base(repository_base&&) noexcept = delete;
            repository_base& operator = (repository_base&&) noexcept = delete;

        public:

            /**
             * Obtains a cached entity, ientified by its primary key.
             * 
             * Note that this method returns a copy of the cached value. Whenever the cache upates, the changes to
             * the entity will not be seen by a previously obtained value.
             *
             * @param[in] id The primary key of the entity to obtain.
             */
            std::optional<ENTITY> operator [] (typename PK::value_type id) const {
                return WithMutex_([&]() -> std::optional<ENTITY> {
                    auto itr = _storage.find(id);
                    if (itr == _storage.end())
                        return std::nullopt;

                    return itr->second;
                });
            }

            /**
             * Executes a callback on each cached entry.
             *
             * @tparam Callback A @p Callable type.
             * @param callback A @p Callable to which each of the cached entries will be provided.
             */
            template <typename Callback>
            void ForEach(Callback&& callback) const {
                return WithMutex_([&]() {
                    for (auto [k, v] : _storage)
                        callback(v);
                });
            }

            /**
             * Executes a callback on each cached entry, early aborting if any invocation if said callable returns @p true.
             * @tparam Callback A @p Callable type.
             * @param callback A @p Callable to which each of the cached entries will be provided.
             * @return @p true if any execution of @p callback returned @p true.
             */
            template <typename Callback>
            bool Any(Callback&& callback) const {
                return WithMutex_([&]() {
                    for (auto [k, v] : _storage)
                        if (callback(v))
                            return true;

                    return false;
                });
            }

            template <typename Callback>
            void WithValues(Callback&& callback) const {
                WithMutex_([&]() {
                    callback(boost::adaptors::values(_storage));
                });
            }

        protected:
            template <typename T>
            void Refresh_(boost::system::error_code const& ec, T* self) {
                if (ec == boost::asio::error::operation_aborted)
                    return;

                LoadFromDB_(self);

                _refreshTimer.expires_at(std::chrono::high_resolution_clock::now() + _refreshInterval);
                _refreshTimer.async_wait([this, self](boost::system::error_code const& ec) {
                    this->Refresh_(ec, self);
                });
            }

            template <typename Callback>
            decltype(auto) WithMutex_(Callback&& callback) const {
                std::unique_lock<std::mutex> lock(_storageMutex);
                return callback();
            }

            /**
            * Reloads cached data from the database.
            *
            * This function automatically executes every minute.
            */
            template <typename T>
            void LoadFromDB_(T* self) {
                namespace chrono = std::chrono;

                _logger.info("Refreshing cache entries for db::{}.", ENTITY::Name.Value);

                spdlog::stopwatch sw { };

                std::vector<ENTITY> storage = LOAD_STATEMENT::Execute(_connection);

                _logger.debug("Loaded {} db::{} entries from database in {}.",
                    storage.size(), ENTITY::Name.Value, chrono::duration_cast<chrono::milliseconds>(sw.elapsed()));

                sw.reset();

                WithMutex_([this, &storage]() {
                    this->_storage.clear();
                    for (auto&& entity : storage)
                        this->_storage.emplace(entity.template get<PK>(), entity);
                    });

                _logger.debug("Cache for db::{} filled in {}.",
                    ENTITY::Name.Value, chrono::duration_cast<chrono::milliseconds>(sw.elapsed()));
            }

        private:
            boost::asio::high_resolution_timer _refreshTimer;
            std::chrono::seconds _refreshInterval;
            mutable std::mutex _storageMutex;

            spdlog::async_logger& _logger;
            boost::container::flat_map<typename PK::value_type, ENTITY> _storage;

        protected:
            pqxx::connection& _connection;
        };

        template <typename LOAD_STATEMENT, typename PK, typename ENTITY> struct repository_base<LOAD_STATEMENT, PK, ENTITY, false> {
            repository_base(spdlog::async_logger& logger, pqxx::connection& connection)
                : _connection(connection)
            { }

        protected:
            template <typename T> void Refresh_(boost::system::error_code const& ec, T* self) { }

            template <typename Callback> decltype(auto) WithMutex_(Callback callback) const { return callback(); }

            pqxx::connection& _connection;
        };
    }

    template <typename ENTITY, typename LOAD_STATEMENT, typename PRIMARY_KEY, bool CACHING = false>
    struct Repository : public detail::repository_base<LOAD_STATEMENT, PRIMARY_KEY, ENTITY, CACHING> {
        template <typename, typename, typename, bool> friend struct detail::repository_base;

        using repository_base = detail::repository_base<LOAD_STATEMENT, PRIMARY_KEY, ENTITY, CACHING>;

    protected:
        template <auto B = CACHING, std::enable_if_t<B, int> _ = 0> // Make dependant
        explicit Repository(utility::ThreadPool& threadPool, pqxx::connection& connection, spdlog::async_logger& logger, std::chrono::seconds interval = 60s)
            : repository_base(threadPool, connection, interval, logger)
        {
        }

        template <auto B = CACHING, std::enable_if_t<!B, int> _ = 0>
        Repository(pqxx::connection& connection, spdlog::async_logger& logger)
            : repository_base(logger, connection)
        {
        }

    public:
        explicit Repository(Repository&& other) noexcept = delete;
        Repository& operator = (Repository&& other) noexcept = delete;
    };
}

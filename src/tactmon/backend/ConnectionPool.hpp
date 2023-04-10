#pragma once

#include <cstdint>
#include <deque>
#include <mutex>
#include <string_view>

#include <pqxx/connection>

namespace backend {
    struct Pool;

    /**
     * A pooled connection to a PostgreSQL database.
     */
    class Connection final {
        friend struct Pool;

        Connection(Pool& pool, pqxx::connection connection) : _pool(pool), _connection(std::move(connection)) { }
        
    public:
        ~Connection();

        pqxx::connection const& connection() const { return _connection; }
        pqxx::connection& connection() { return _connection; }

    private:
        Pool& _pool;
        pqxx::connection _connection;
    };

    /**
     * A pool of connections to a PostgreSQL database.
     */
    struct Pool final {
        friend class Connection;

        Pool(std::string_view username, std::string_view password, std::string_view host, uint64_t port, std::string_view name);

        void Prepare(std::string_view name, std::string_view definition);

        Connection Open();

        std::size_t size() const { return _poolCapacity; }

    private:
        void Return(pqxx::connection connection);

        std::deque<pqxx::connection> _pool;
        std::mutex _mutex;
        std::condition_variable _condition;

        std::size_t _poolCapacity = 10;
    };

    inline Connection::~Connection() {
        _pool.Return(std::move(_connection));
    }
}

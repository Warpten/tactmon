#include "backend/ConnectionPool.hpp"

#include <fmt/format.h>

namespace backend {
    Pool::Pool(std::string_view username, std::string_view password, std::string_view host, uint64_t port, std::string_view name) {
        std::string connectionString = fmt::format("user={} password={} host={} port={} dbname={} target_session_attrs=read-write",
            username, password, host, port, name);

        std::lock_guard<std::mutex> guard(_mutex);
        for (std::size_t i = 0; i < _poolCapacity; ++i)
            _pool.emplace_back(connectionString);
    }

    void Pool::Prepare(std::string_view name, std::string_view definition) {
        std::lock_guard<std::mutex> guard(_mutex);
        for (std::size_t i = 0; i < _poolCapacity; ++i)
            _pool[i].prepare(pqxx::zview{ name }, pqxx::zview { definition });
    }

    /* static */ Connection Pool::Open() {
        std::unique_lock<std::mutex> l(_mutex);

        while (_pool.empty())
            _condition.wait(l);

        pqxx::connection connection = std::move(_pool.front());
        _pool.pop_front();

        return Connection { *this, std::move(connection) };
    }

    void Pool::Return(pqxx::connection connection) {
        std::unique_lock<std::mutex> l(_mutex);
        _pool.push_back(std::move(connection));

        l.unlock();
        _condition.notify_one();
    }
}

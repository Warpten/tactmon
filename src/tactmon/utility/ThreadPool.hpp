#pragma once

#include <type_traits>

#include <boost/asio/thread_pool.hpp>
#include <boost/asio/executor_work_guard.hpp>

namespace utility {
    template <typename Executor>
    struct ThreadPool final {
        template <typename... Args>
        requires std::is_constructible_v<Executor, Args...>
        explicit ThreadPool(Args&&... args) noexcept : _service(std::forward<Args>(args)...) { }

        void PostWork(std::function<void(Executor&, boost::asio::executor_work_guard<Executor> const&)> work) {
            boost::asio::post(_pool, [&]() {
                work(_service, _guard);
            });
        }

        ~ThreadPool() {
            _guard.reset();

            _pool.join();
        }

    private:
        Executor _service;
        boost::asio::thread_pool _pool;
        boost::asio::executor_work_guard<Executor> _guard;
    };
}

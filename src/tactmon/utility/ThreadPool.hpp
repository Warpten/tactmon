#pragma once

#include <type_traits>

#include <boost/asio/any_io_executor.hpp>
#include <boost/asio/executor_work_guard.hpp>
#include <boost/asio/io_context.hpp>
#include <boost/asio/post.hpp>
#include <boost/asio/thread_pool.hpp>

namespace utility {
    struct ThreadPool final {
        explicit ThreadPool(size_t threadCount) noexcept : _size(threadCount), _service(threadCount), _pool(threadCount), _guard(boost::asio::make_work_guard(_service)) {
        }

        void PostWork(std::function<void(boost::asio::io_context&, boost::asio::executor_work_guard<boost::asio::io_context::executor_type> const&)> work) {
            boost::asio::post(_pool, [&]() {
                work(_service, _guard);
            });
        }

        size_t size() const { return _size; }

        void Interrupt() {
            _guard.reset();
        }

        void PostWork(std::function<void()> work) {
            boost::asio::post(_pool, work);
        }

        ~ThreadPool() {
            Interrupt();
            _pool.join();
        }

        boost::asio::io_context& service() { return _service; }
        boost::asio::any_io_executor executor() { return _pool.get_executor(); }

    private:
        size_t _size;
        boost::asio::io_context _service;
        boost::asio::thread_pool _pool;
        boost::asio::executor_work_guard<boost::asio::io_context::executor_type> _guard;
    };
}

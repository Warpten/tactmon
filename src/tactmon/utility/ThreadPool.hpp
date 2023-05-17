#pragma once

#include <iostream>
#include <type_traits>

#include <boost/asio/any_io_executor.hpp>
#include <boost/asio/executor_work_guard.hpp>
#include <boost/asio/io_context.hpp>
#include <boost/asio/post.hpp>
#include <boost/asio/thread_pool.hpp>

namespace utility {
    struct ThreadPool final {
        explicit ThreadPool(std::size_t threadCount) noexcept : _size(threadCount),
            service(threadCount),
            _pool(threadCount),
            _guard(boost::asio::make_work_guard(service))
        { }

        void PostWork(std::function<void(boost::asio::io_context&, boost::asio::executor_work_guard<boost::asio::io_context::executor_type> const&)> work) {
            boost::asio::post(_pool, [&, work = std::move(work)]() {
                try {
                    work(service, _guard);
                } catch (std::exception const& ex) {
                    std::cerr << ex.what() << std::endl;
                }
            });
        }

        [[nodiscard]] std::size_t size() const { return _size; }

        void Interrupt() {
            _guard.reset();
            _pool.stop();
            _pool.join();
        }

        void PostWork(std::function<void()> work) {
            boost::asio::post(_pool, work);
        }

        void PostWork(std::function<void(boost::asio::io_context&)> work) {
            boost::asio::post(_pool, [&, work = std::move(work)]() {
                try {
                    work(service);
                } catch (std::exception const& ex) {
                    std::cerr << ex.what() << std::endl;
                }
            });
        }

        ~ThreadPool() {
            Interrupt();
        }

        boost::asio::any_io_executor pool_executor() { return _pool.get_executor(); }

    private:
        std::size_t _size;
    public:
        boost::asio::io_context service;
    private:
        boost::asio::thread_pool _pool;
        boost::asio::executor_work_guard<boost::asio::io_context::executor_type> _guard;
    };
}

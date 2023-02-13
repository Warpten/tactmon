#pragma once

#include <boost/asio/thread_pool.hpp>

struct ThreadPool final {
    explicit ThreadPool(size_t numThreads = std::thread::hardware_concurrency()) : _implementation(numThreads) { }

    template <typename T>
    decltype(auto) Submit(T&& work) {
        return boost::asio::post(_implementation, std::forward<T&&>(work));
    }

    void Join() {
        _implementation.join();
    }

private:
    boost::asio::thread_pool _implementation;
};

#pragma once

#include "backend/Database.hpp"
#include "backend/ProductCache.hpp"

#include <cstdint>
#include <functional>
#include <optional>
#include <string>
#include <vector>

#include <boost/asio/high_resolution_timer.hpp>
#include <boost/asio/io_service.hpp>
#include <boost/system/error_code.hpp>

namespace backend {
    struct RibbitMonitor final {
        enum class ProductState : uint8_t {
            Updated,
            Deleted // NYI
        };

        using Listener = std::function<void(std::string const&, uint64_t, ProductState)>;

        explicit RibbitMonitor(boost::asio::io_context& context, backend::Database& db);

        void BeginUpdate();

        void RegisterListener(Listener&& listener);

    private:
        void OnUpdate(boost::system::error_code ec);
        void NotifyProductUpdate(std::string const& productName, uint32_t sequenceID) const;
        void NotifyProductDeleted(std::string const& productName, uint32_t sequenceID) const;

    private:
        backend::Database& _database;

        boost::asio::io_context& _service;
        boost::asio::high_resolution_timer _timer;

        std::vector<Listener> _listeners;

    };
}

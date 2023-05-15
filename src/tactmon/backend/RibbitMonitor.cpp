#include "backend/RibbitMonitor.hpp"
#include "backend/db/repository/Product.hpp"

#include <chrono>
#include <optional>

#include <libtactmon/ribbit/Commands.hpp>
#include <libtactmon/ribbit/types/Summary.hpp>

namespace ribbit = libtactmon::ribbit;

namespace backend {
    RibbitMonitor::RibbitMonitor(boost::asio::any_io_executor executor, backend::Database& db) : _database(db), _executor(std::move(executor)), _timer(executor)
    {
    }

    void RibbitMonitor::BeginUpdate()
    {
        OnUpdate({ });

        using namespace std::chrono_literals;

        _timer.expires_at(std::chrono::high_resolution_clock::now() + 60s);
        _timer.async_wait([=, this](boost::system::error_code ec) {
            this->OnUpdate(ec);

            this->BeginUpdate();
        });
    }

    void RibbitMonitor::OnUpdate(boost::system::error_code ec) {
        if (ec == boost::asio::error::operation_aborted)
            return;

        auto summary = ribbit::Summary<>::Execute(_executor, ribbit::Region::US);
        if (!summary.has_value())
            return;

        for (ribbit::types::summary::Record const& record : summary.value()) {
            if (!record.Flags.empty())
                continue;

            std::optional<db::entity::product::Entity> productState = _database.products->GetByName(record.Product);
            if (!productState.has_value()) {
                _database.products->Insert(record.Product, record.SequenceID);

                NotifyProductUpdate(record.Product, record.SequenceID);
            }
            else {
                uint64_t previousSequenceID = db::get<db::entity::product::sequence_id>(productState.value());

                if (previousSequenceID < record.SequenceID) {
                    // New SeqN, update database.
                    _database.products->Update(record.Product, record.SequenceID);

                    NotifyProductUpdate(record.Product, record.SequenceID);
                }
            }
        }
    }

    void RibbitMonitor::RegisterListener(Listener&& listener) {
        _listeners.push_back(std::move(listener));
    }

    void RibbitMonitor::NotifyProductUpdate(std::string const& productName, uint32_t sequenceID) const {
        for (Listener const& listener : _listeners)
            listener(productName, sequenceID, ProductState::Updated);
    }

    void RibbitMonitor::NotifyProductDeleted(std::string const& productName, uint32_t sequenceID) const {
        for (Listener const& listener : _listeners)
            listener(productName, sequenceID, ProductState::Deleted);
    }
}

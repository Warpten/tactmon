#include "tact/data/product/Product.hpp"
#include "net/ribbit/Commands.hpp"

#include "logging/Sinks.hpp"

#include <filesystem>

namespace tact::data::product {
    Product::Product(std::string_view productName, boost::asio::io_context& ctx) : _context(ctx), _productName(productName) {

    }

    bool Product::Refresh() noexcept {
        using namespace std::string_view_literals;

        namespace ribbit = net::ribbit;
        auto ribbitLogger = logging::GetLogger("ribbit");

        ribbit::CommandExecutor<ribbit::Command::Summary, ribbit::Region::EU, ribbit::Version::V1> summaryCommand{ _context };
        std::optional<ribbit::types::Summary> summary = summaryCommand(); // Call op
        if (!summary.has_value())
            return false;

        // Find expected sequence number
        auto summaryItr = std::find_if(summary->begin(), summary->end(), [productName = _productName](ribbit::types::summary::Record const& record) {
            return record.Product == productName && record.Flags.empty();
        });
        if (summaryItr == summary->end())
            return false;

        ribbit::CommandExecutor<ribbit::Command::ProductVersions, ribbit::Region::EU, ribbit::Version::V1> versionsCommand{ _context };
        std::optional<ribbit::types::Versions> versions = versionsCommand("wow"sv); // Call op
        if (!versions.has_value())
            return false;

        if (versions->SequenceID != summaryItr->SequenceID) {
            ribbitLogger->error("Received stale version (expected {}, got {})", summaryItr->SequenceID, versions->SequenceID);

            return false;
        }

        ribbit::CommandExecutor<ribbit::Command::ProductCDNs, ribbit::Region::EU, ribbit::Version::V1> cdnsCommand{ _context };
        std::optional<ribbit::types::CDNs> cdns = cdnsCommand("wow"sv);
        if (!cdns.has_value())
            return false;

        _localInstance.emplace(std::filesystem::current_path(), *cdns, versions->Records[0], _context);
        return LoadRoot();
    }
}
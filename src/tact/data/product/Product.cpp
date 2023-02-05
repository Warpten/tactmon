#include "logging/Sinks.hpp"
#include "net/ribbit/Commands.hpp"
#include "net/DownloadTask.hpp"
#include "io/mem/MemoryStream.hpp"
#include "tact/EKey.hpp"
#include "tact/data/Encoding.hpp"
#include "tact/data/product/Product.hpp"

#include <filesystem>

namespace tact::data::product {
    Product::Product(std::string_view productName, std::filesystem::path installationRoot,
        boost::asio::io_context& context)
        : _context(context), _productName(productName), _installRoot(installationRoot)
    {
    }

    bool Product::Refresh() noexcept {
        using namespace std::string_view_literals;

        namespace ribbit = net::ribbit;
        auto ribbitLogger = logging::GetLogger("ribbit");

        ribbit::Summary<ribbit::Region::EU, ribbit::Version::V1> summaryCommand { _context };
        std::optional<ribbit::types::Summary> summary = summaryCommand(); // Call op
        if (!summary.has_value())
            return false;

        auto summaryItr = std::find_if(summary->begin(), summary->end(), [productName = _productName](ribbit::types::summary::Record const& record) {
            return record.Product == productName && record.Flags.empty();
        });
        if (summaryItr == summary->end())
            return false;

        ribbit::Versions<ribbit::Region::EU, ribbit::Version::V1> versionsCommand { _context };
        std::optional<ribbit::types::Versions> versions = versionsCommand("wow"sv); // Call op
        if (!versions.has_value())
            return false;

#if !_DEBUG
        // Validate seqn for stale version information
        if (versions->SequenceID != summaryItr->SequenceID) {
            ribbitLogger->error("Received stale version (expected {}, got {})", summaryItr->SequenceID, versions->SequenceID);

            return false;
        }
#endif

        ribbit::CDNs<ribbit::Region::EU> cdnsCommand { _context };
        _cdns = cdnsCommand("wow"sv);
        if (!_cdns.has_value())
            return false;

        // Select one version at random
        _version = versions->Records[0];

        _logger = logging::GetLogger(_version->VersionsName);

        _logger->info("Detected build configuration: {}.", _version->BuildConfig);
        _logger->info("Detected CDN configuration: {}.", _version->CDNConfig);
        _logger->info("Detected product configuration: {}.", _version->ProductConfig);

        _buildConfig = ResolveConfig<tact::config::BuildConfig>(_version->BuildConfig, [](io::IReadableStream& fstream) -> std::optional<tact::config::BuildConfig> {
            return tact::config::BuildConfig { fstream };
        });

        if (!_buildConfig.has_value()) {
            _logger->error("An error occured while parsing build configuration.");
            return false;
        }

        _cdnConfig = ResolveConfig<tact::config::CDNConfig>(_version->CDNConfig, [](io::IReadableStream& fstream) -> std::optional<tact::config::CDNConfig> {
            return tact::config::CDNConfig { fstream };
        });

        if (!_cdnConfig.has_value()) {
            _logger->error("An error occured while parsing CDN configuration.");
            return false;
        }

        _logger->info("Detected encoding manifest: {}.", _buildConfig->Encoding.Key.EncodingKey.ToString());
        _logger->info("Detected install manifest: {}.", _buildConfig->Install.Key.EncodingKey.ToString());

        _encoding = ResolveCachedData<tact::data::Encoding>(_buildConfig->Encoding.Key.EncodingKey.ToString(),
            [&key = _buildConfig->Encoding.Key](io::FileStream& fstream) -> std::optional<tact::data::Encoding>
            {
                if (!fstream)
                    return std::nullopt;

                std::optional<tact::BLTE> compressedArchive = tact::BLTE::Parse(fstream, key.EncodingKey, key.ContentKey);
                if (!compressedArchive.has_value())
                    return std::nullopt;

                return tact::data::Encoding { compressedArchive->GetStream() };
            });

        if (!_encoding.has_value()) {
            _logger->error("An error occured while parsing encoding manifest.");
            return false;
        }

        _logger->info("{} entries found in encoding manifest.", _encoding->count());

        _install = ResolveCachedData<tact::data::Install>(_buildConfig->Install.Key.EncodingKey.ToString(),
            [&key = _buildConfig->Install.Key](io::FileStream& fstream) -> std::optional<tact::data::Install>
            {
                if (!fstream)
                    return std::nullopt;

                std::optional<tact::BLTE> compressedArchive = tact::BLTE::Parse(fstream, key.EncodingKey, key.ContentKey);
                if (!compressedArchive.has_value())
                    return std::nullopt;

                return tact::data::Install::Parse(compressedArchive->GetStream());
            });

        if (!_install.has_value()) {
            _logger->error("An error occured while parsing install manifest.");
            return false;
        }

        _logger->info("{} entries found in install manifest.", _install->size());

        _cdnConfig->ForEachArchive([this](std::string_view archiveName, size_t i) {
            auto dataStream = ResolveCachedData<io::FileStream>(std::format("{}.index", archiveName), [](io::FileStream& fstream) -> std::optional<io::FileStream> {
                if (!fstream)
                    return std::nullopt;

                return fstream;
            });

            if (dataStream.has_value())
                _indices.emplace_back(archiveName, *dataStream);
        });

        return true;
    }

    std::optional<tact::data::FileLocation> Product::FindFile(std::string_view fileName) const {
        std::optional<tact::CKey> installKey = _install->FindFile(fileName);
        if (installKey.has_value())
            return FindFile(*installKey);

        return std::nullopt;
    }

    std::optional<tact::data::FileLocation> Product::FindFile(tact::CKey const& contentKey) const {
        if (!_encoding.has_value())
            return std::nullopt;

        return _encoding->FindFile(contentKey);
    }

    std::optional<tact::BLTE> Product::Open(tact::data::FileLocation const& location) const {
        for (size_t i = 0; i < location.keyCount(); ++i) {
            tact::EKey encodingKey { location[i] };

            // Try via indexes
            std::optional<tact::data::IndexFileLocation> indexLocation = FindIndex(encodingKey);
            if (indexLocation.has_value()) {
                auto dataStream = ResolveData<net::MemoryDownloadTask, tact::BLTE>([&indexLocation]() -> net::MemoryDownloadTask {
                    return net::MemoryDownloadTask { indexLocation->name(), indexLocation->offset(), indexLocation->fileSize() };
                },
                [](net::MemoryDownloadTask::ResultType& result) -> std::optional<tact::BLTE> {
                    if (result.has_value())
                        return tact::BLTE::Parse(*result);
                    return std::nullopt;
                });

                if (dataStream.has_value())
                    return dataStream;
            }

            // Otherwise try to load the ekey as a file from CDN directly
            auto dataStream = ResolveData<net::MemoryDownloadTask, tact::BLTE>([&encodingKey]() {
                return net::MemoryDownloadTask { encodingKey.ToString() };
            }, [](net::MemoryDownloadTask::ResultType& result) -> std::optional<tact::BLTE> {
                if (result.has_value())
                    return tact::BLTE::Parse(*result);
                return std::nullopt;
            });

            if (dataStream.has_value())
                return dataStream;
        }

        return std::nullopt;
    }

    std::optional<tact::data::IndexFileLocation> Product::FindIndex(tact::EKey const& ekey) const {
        for (tact::data::Index const& index : _indices) {
            tact::data::Index::Entry const* entry = index[ekey];
            if (entry != nullptr)
                return tact::data::IndexFileLocation{ index.name(), entry->offset(), entry->size() };
        }

        return std::nullopt;
    }
}

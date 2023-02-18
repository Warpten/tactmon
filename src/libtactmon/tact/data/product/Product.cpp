#include "logging/Sinks.hpp"
#include "net/ribbit/Commands.hpp"
#include "net/DownloadTask.hpp"
#include "io/mem/MemoryStream.hpp"
#include "tact/EKey.hpp"
#include "tact/data/Encoding.hpp"
#include "tact/data/product/Product.hpp"

#include <filesystem>

namespace tact::data::product {
    using namespace std::string_view_literals;
    namespace ribbit = net::ribbit;

    Product::Product(std::string_view productName, Cache& localCache, boost::asio::io_context& context)
        : _context(context), _productName(productName), _localCache(localCache)
    {
    }

    bool Product::Load(std::string_view buildConfig, std::string_view cdnConfig) noexcept {
        // **Always** refresh CDN
        ribbit::CDNs<ribbit::Region::EU> cdnsCommand{ _context };
        _cdns = cdnsCommand("wow"sv);
        if (!_cdns.has_value())
            return false;

        // Load build config and cdn config; abort if invalid or not found.
        _buildConfig = ResolveCachedConfig<tact::config::BuildConfig>(buildConfig, [](io::FileStream& fstream) -> std::optional<tact::config::BuildConfig> {
            return tact::config::BuildConfig { fstream };
        });
        if (!_buildConfig.has_value())
            return false;

        _cdnConfig = ResolveCachedConfig<tact::config::CDNConfig>(cdnConfig, [](io::FileStream& fstream) -> std::optional<tact::config::CDNConfig> {
            return tact::config::CDNConfig{ fstream };
        });
        if (!_cdnConfig.has_value())
            return false;
        
        // Begin loading here.
        _logger = logging::GetLogger(_buildConfig->BuildName);

        _logger->info("Detected encoding manifest: {}.", _buildConfig->Encoding.Key.EncodingKey.ToString());
        _logger->info("Detected install manifest: {}.", _buildConfig->Install.Key.EncodingKey.ToString());
        _logger->info("Detected root manifest: {}.", _buildConfig->Root.ToString());

        _encoding = ResolveCachedData<tact::data::Encoding>(_buildConfig->Encoding.Key.EncodingKey.ToString(),
            [&key = _buildConfig->Encoding.Key](io::FileStream& fstream) -> std::optional<tact::data::Encoding>
            {
                if (!fstream)
                    return std::nullopt;

                std::optional<tact::BLTE> compressedArchive = tact::BLTE::Parse(fstream, key.EncodingKey, key.ContentKey);
                if (!compressedArchive.has_value())
                    return std::nullopt;

                return tact::data::Encoding{ compressedArchive->GetStream() };
            });

        if (!_encoding.has_value()) {
            _logger->error("An error occured while parsing encoding manifest.");
            return false;
        }

        _logger->info("{} entries found in encoding manifest.", _encoding->count());

        _install = ResolveCachedData<tact::data::Install>(_buildConfig->Install.Key.EncodingKey.ToString(),
            [&key = _buildConfig->Install.Key](io::FileStream& fstream) -> std::optional<tact::data::Install> {
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
            auto dataStream = ResolveCachedData<io::FileStream>(fmt::format("{}.index", archiveName),
                [](io::FileStream& fstream) -> std::optional<io::FileStream> {
                    if (!fstream)
                        return std::nullopt;
            
                    return fstream;
                });
            
            if (dataStream.has_value())
                _indices.emplace_back(archiveName, *dataStream);
        });

        return true;
    }

    std::optional<net::ribbit::types::Versions> Product::Refresh() noexcept {
        auto ribbitLogger = logging::GetLogger("ribbit");

        ribbit::Summary<ribbit::Region::EU, ribbit::Version::V1> summaryCommand { _context };
        std::optional<ribbit::types::Summary> summary = summaryCommand(); // Call op
        if (!summary.has_value())
            return std::nullopt;

        auto summaryItr = std::find_if(summary->begin(), summary->end(), [productName = _productName](ribbit::types::summary::Record const& record) {
            return record.Product == productName && record.Flags.empty();
        });
        if (summaryItr == summary->end())
            return std::nullopt;

        ribbit::Versions<ribbit::Region::EU, ribbit::Version::V1> versionsCommand { _context };
        std::optional<ribbit::types::Versions> versions = versionsCommand(std::string_view { _productName }); // Call op
        if (!versions.has_value())
            return std::nullopt;

#if !_DEBUG
        // Validate seqn for stale version information
        if (versions->SequenceID != summaryItr->SequenceID) {
            ribbitLogger->error("Received stale version (expected {}, got {})", summaryItr->SequenceID, versions->SequenceID);

            return std::nullopt;
        }
#endif

        return versions;
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
            std::optional<tact::data::ArchiveFileLocation> indexLocation = FindArchive(encodingKey);
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

    std::optional<tact::data::ArchiveFileLocation> Product::FindArchive(tact::EKey const& ekey) const {
        for (tact::data::Index const& index : _indices) {
            tact::data::Index::Entry const* entry = index[ekey];
            if (entry != nullptr)
                return tact::data::ArchiveFileLocation{ index.name(), entry->offset(), entry->size() };
        }

        return std::nullopt;
    }
}

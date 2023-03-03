#include "libtactmon/io/MemoryStream.hpp"
#include "libtactmon/net/DownloadTask.hpp"
#include "libtactmon/net/MemoryDownloadTask.hpp"
#include "libtactmon/ribbit/Commands.hpp"
#include "libtactmon/tact/data/Encoding.hpp"
#include "libtactmon/tact/data/product/Product.hpp"
#include "libtactmon/tact/EKey.hpp"

#include <filesystem>
#include <future>

#include <boost/thread/future.hpp>

#include <fmt/std.h>

namespace libtactmon::tact::data::product {
    using namespace std::string_view_literals;

    Product::Product(std::string_view productName, Cache& localCache, boost::asio::any_io_executor executor, std::shared_ptr<spdlog::logger> logger)
        : ResourceResolver(executor, localCache), _executor(executor), _productName(productName), _localCache(localCache), _logger(logger)
    {
    }

    bool Product::Load(std::string_view buildConfig, std::string_view cdnConfig) noexcept {
        // **Always** refresh CDN
        _cdns = ribbit::CDNs<>::Execute(_executor, nullptr, ribbit::Region::US, std::string_view { _productName });
        if (!_cdns.has_value())
            return false;

        // Load build config and cdn config; abort if invalid or not found.
        _buildConfig = ResolveCachedConfig<tact::config::BuildConfig>(buildConfig, [](io::FileStream& fstream) {
            return tact::config::BuildConfig::Parse(fstream);
        });
        if (!_buildConfig.has_value())
            return false;

        _cdnConfig = ResolveCachedConfig<tact::config::CDNConfig>(cdnConfig, [](io::FileStream& fstream) {
            return tact::config::CDNConfig::Parse(fstream);
        });
        if (!_cdnConfig.has_value())
            return false;
        
        // Begin loading here.

        if (_logger != nullptr) {
            _logger->info("({}) Detected encoding manifest: {}.", _buildConfig->BuildName, _buildConfig->Encoding.Key.EncodingKey.ToString());
            _logger->info("({}) Detected install manifest: {}.", _buildConfig->BuildName, _buildConfig->Install.Key.EncodingKey.ToString());
            _logger->info("({}) Detected root manifest: {}.", _buildConfig->BuildName, _buildConfig->Root.ToString());
        }

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
            if (_logger != nullptr)
                _logger->error("({}) An error occured while parsing encoding manifest.", _buildConfig->BuildName);
            return false;
        }

        if (_logger != nullptr)
            _logger->info("({}) {} entries found in encoding manifest.", _buildConfig->BuildName, _encoding->count());

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
            if (_logger != nullptr)
                _logger->error("({}) An error occured while parsing install manifest.", _buildConfig->BuildName);
            return false;
        }

        if (_logger != nullptr)
            _logger->info("({}) {} entries found in install manifest.", _buildConfig->BuildName, _install->size());

        using index_parse_task = boost::packaged_task<std::optional<tact::data::Index>>;
        std::list<boost::future<std::optional<tact::data::Index>>> archiveFutures;
        _cdnConfig->ForEachArchive([&](std::string_view archiveName, size_t archiveSize)
        {
            std::shared_ptr<index_parse_task> task = std::make_shared<index_parse_task>([=]() -> std::optional<tact::data::Index> {
                if (_logger != nullptr)
                    _logger->info("({}) Loading archive '{}'. {}", _buildConfig->BuildName, archiveName, std::this_thread::get_id());

                auto dataStream = ResolveCachedData<io::FileStream>(fmt::format("{}.index", archiveName),
                    [archiveSize](io::FileStream& fstream) -> std::optional<io::FileStream> {
                        if (!fstream/* || fstream.GetLength() != archiveSize*/)
                            return std::nullopt;

                        return fstream;
                    });

                if (dataStream.has_value())
                    return tact::data::Index{ archiveName, dataStream.value() };

                return std::nullopt;
            });

            archiveFutures.push_back(task->get_future());

            boost::asio::post(_executor, [task]() { (*task)(); });
        });

        for (boost::future<std::optional<tact::data::Index>>& future : boost::when_all(archiveFutures.begin(), archiveFutures.end()).get()) {
            std::optional<tact::data::Index> futureOutcome = future.get();
            if (futureOutcome.has_value())
                _indices.push_back(std::move(futureOutcome.value()));
        }

        return true;
    }

    std::optional<ribbit::types::Versions> Product::Refresh() noexcept {
        auto summary = ribbit::Summary<>::Execute(_executor, _logger, ribbit::Region::US);
        if (!summary.has_value())
            return std::nullopt;

        auto summaryItr = std::find_if(summary->begin(), summary->end(), [productName = _productName](ribbit::types::summary::Record const& record) {
            return record.Product == productName && record.Flags.empty();
        });
        if (summaryItr == summary->end())
            return std::nullopt;

        auto versions = ribbit::Versions<>::Execute(_executor, _logger, ribbit::Region::US, std::string_view { _productName });
        if (!versions.has_value())
            return std::nullopt;

#if !_DEBUG
        // Validate seqn for stale version information
        if (versions->SequenceID != summaryItr->SequenceID) {
            if (_logger != nullptr)
                _logger->error("Received stale version (expected {}, got {})", summaryItr->SequenceID, versions->SequenceID);

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

    std::optional<tact::data::ArchiveFileLocation> Product::FindArchive(tact::EKey const& ekey) const {
        for (tact::data::Index const& index : _indices) {
            tact::data::Index::Entry const* entry = index[ekey];
            if (entry != nullptr)
                return tact::data::ArchiveFileLocation{ index.name(), entry->offset(), entry->size() };
        }

        return std::nullopt;
    }
}

#include "libtactmon/io/MemoryStream.hpp"
#include "libtactmon/ribbit/Commands.hpp"
#include "libtactmon/tact/data/Encoding.hpp"
#include "libtactmon/tact/data/product/Product.hpp"
#include "libtactmon/tact/EKey.hpp"
#include "libtactmon/utility/Formatting.hpp"
#include "libtactmon/Errors.hpp"

#include <filesystem>
#include <future>

#include <boost/asio/thread_pool.hpp>
#include <boost/thread/future.hpp>

#include <fmt/std.h>

namespace libtactmon::tact::data::product {
    using namespace std::string_view_literals;

    Product::Product(std::string_view productName, Cache& localCache, boost::asio::any_io_executor executor, std::shared_ptr<spdlog::logger> logger)
        : ResourceResolver(std::move(executor), localCache), _productName(productName), _localCache(localCache), _logger(std::move(logger))
    {
    }

    bool Product::Load(std::string_view buildConfig, std::string_view cdnConfig) noexcept {
        // **Always** refresh CDN
        _cdns = ribbit::CDNs<>::Execute(_executor, ribbit::Region::US, _productName).ToOptional();
        if (!_cdns.has_value())
            return false;

        // Load build config and cdn config; abort if invalid or not found.
        _buildConfig = ResolveCachedConfig(buildConfig).transform([](io::FileStream fstream) {
            return tact::config::BuildConfig::Parse(fstream);
        }).ToOptional();
        if (!_buildConfig.has_value())
            return false;

        _cdnConfig = ResolveCachedConfig(cdnConfig).transform([](io::FileStream fstream) {
            return tact::config::CDNConfig::Parse(fstream);
        }).ToOptional();
        if (!_cdnConfig.has_value())
            return false;
        
        // Begin loading here.

        if (_logger != nullptr) {
            _logger->info("({}) Detected encoding manifest: {}.", _buildConfig->BuildName, _buildConfig->Encoding.Key.EncodingKey.ToString());
            _logger->info("({}) Detected install manifest: {}.", _buildConfig->BuildName, _buildConfig->Install.Key.EncodingKey.ToString());
            _logger->info("({}) Detected root manifest: {}.", _buildConfig->BuildName, _buildConfig->Root.ToString());
        }

        _encoding = [&]() -> std::optional<tact::data::Encoding> {
            Result<tact::data::Encoding> encoding = ResolveCachedBLTE(_buildConfig->Encoding.Key.EncodingKey, _buildConfig->Encoding.Key.ContentKey)
                .and_then([](io::GrowableMemoryStream decompressedStream) {
                    return tact::data::Encoding { decompressedStream };
                });

            if (encoding)
                return std::move(encoding).ToOptional();

            if (_logger != nullptr)
                _logger->error("({}) An error occured while loading the encoding manifest: {}.", _buildConfig->BuildName, encoding.code());

            return std::nullopt;
        }();
        if (!_encoding.has_value())
            return false;

        if (_logger != nullptr)
            _logger->info("({}) {} entries found in encoding manifest.", _buildConfig->BuildName, _encoding->count());

        _install = [&]() -> std::optional<tact::data::Install> {
            Result<tact::data::Install> install = ResolveCachedBLTE(_buildConfig->Install.Key.EncodingKey, _buildConfig->Install.Key.ContentKey)
                .transform([](io::GrowableMemoryStream decompressedStream) {
                    return tact::data::Install::Parse(decompressedStream);
                });

            if (install)
                return std::move(install).ToOptional();

            if (_logger != nullptr)
                _logger->error("({}) An error occured while parsing install manifest: {}.", _buildConfig->BuildName, install.code());

            return std::nullopt;
        }();
        if (!_install.has_value())
            return false;

        if (_logger != nullptr)
            _logger->info("({}) {} entries found in install manifest.", _buildConfig->BuildName, _install->size());

        using index_parse_task = boost::packaged_task<std::optional<tact::data::Index>>;
        std::list<boost::future<std::optional<tact::data::Index>>> archiveFutures;

        for (config::CDNConfig::Archive const& archive : _cdnConfig->archives) {
            std::shared_ptr<index_parse_task> task = std::make_shared<index_parse_task>(
                [archiveName = archive.Name, archiveSize = archive.Size, buildName = _buildConfig->BuildName, this]() {
                    return ResolveCachedData(fmt::format("{}.index", archiveName))
                        .transform([&](io::FileStream fstream) {
                            return tact::data::Index::TryParse(archiveName, fstream);
                        }).ToOptional();
                }
            );

            archiveFutures.push_back(task->get_future());

            boost::asio::post(_executor, [task]() { (*task)(); });
        }

        if (_cdnConfig->fileIndex.has_value()) {
            if (_logger != nullptr)
                _logger->info("({}) Loading file index '{}'.", _buildConfig->BuildName, _cdnConfig->fileIndex->Name);

            _fileIndex = ResolveCachedData(fmt::format("{}.index", _cdnConfig->fileIndex->Name)).transform(
                [name = _cdnConfig->fileIndex->Name](io::FileStream fstream) {
                    return tact::data::Index::TryParse(name, fstream);
                }).ToOptional();
        }

        _indices.reserve(archiveFutures.size());
        for (boost::future<std::optional<tact::data::Index>>& future : boost::when_all(archiveFutures.begin(), archiveFutures.end()).get()) {
            std::optional<tact::data::Index> futureOutcome = future.get();
            if (futureOutcome.has_value())
                _indices.push_back(std::move(futureOutcome.value()));
        }

        return true;
    }

    std::optional<ribbit::types::Versions> Product::Refresh() noexcept {
        auto summary = ribbit::Summary<>::Execute(_executor, ribbit::Region::US);
        if (!summary.has_value())
            return std::nullopt;

        auto summaryItr = std::find_if(summary->begin(), summary->end(), [productName = _productName](ribbit::types::summary::Record const& record) {
            return record.Product == productName && record.Flags.empty();
        });
        if (summaryItr == summary->end())
            return std::nullopt;

        auto versions = ribbit::Versions<>::Execute(_executor, ribbit::Region::US, _productName);
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

        return std::move(versions).ToOptional();
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
        // Fast path for non-archived files
        if (_fileIndex.has_value()) {
            tact::data::Index::Entry const* entry = (*_fileIndex)[ekey];
            if (entry != nullptr)
                return tact::data::ArchiveFileLocation { ekey.ToString(), entry->offset(), entry->size() };
        }

        for (tact::data::Index const& index : _indices) {
            tact::data::Index::Entry const* entry = index[ekey];
            if (entry != nullptr)
                return tact::data::ArchiveFileLocation { index.name(), entry->offset(), entry->size() };
        }

        return std::nullopt;
    }
}

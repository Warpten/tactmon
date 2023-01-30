#include "io/LocalCache.hpp"
#include "tact/BLTE.hpp"

#include <spdlog/spdlog.h>

#include <fstream>

namespace io {
    LocalCache::LocalCache(std::filesystem::path installationRoot, net::ribbit::types::CDNs const& cdns, net::ribbit::types::versions::Record const& version, boost::asio::io_context& ctx)
        : _installRoot(installationRoot), _cdns(cdns), _version(version), _context(ctx)
    {
        _logger = logging::GetLogger(version.VersionsName);
        _logger->set_level(spdlog::level::info);

        _logger->info("Detected build configuration: {}.", _version.BuildConfig);
        _logger->info("Detected CDN configuration: {}.", _version.CDNConfig);
        _logger->info("Detected product configuration: {}.", _version.ProductConfig);

        _buildConfig = ResolveConfig<tact::config::BuildConfig>(_version.BuildConfig, [](io::IReadableStream& fstream) -> std::optional<tact::config::BuildConfig> {
            return tact::config::BuildConfig { fstream };
        });

        if (!_buildConfig.has_value()) {
            _logger->error("An error occured while parsing build configuration");
            return;
        }

        _logger->info("Detected encoding manifest: {}.", _buildConfig->Encoding.Key.EncodingKey.ToString());
        _logger->info("Detected install manifest: {}.", _buildConfig->Install.Key.EncodingKey.ToString());

        _encoding = ResolveData<tact::data::Encoding>(_buildConfig->Encoding.Key.EncodingKey.ToString(),
            [&key = _buildConfig->Encoding.Key](io::IReadableStream& fstream) -> std::optional<tact::data::Encoding>
            {
                if (!fstream)
                    return std::nullopt;

                std::optional<tact::BLTE> compressedArchive = tact::BLTE::Parse(fstream, key.EncodingKey, key.ContentKey);
                if (!compressedArchive.has_value())
                    return std::nullopt;

                return tact::data::Encoding { compressedArchive->GetStream() };
            }
        );

        if (!_encoding.has_value()) {
            _logger->error("An error occured while parsing encoding manifest.");
            return;
        }

        _logger->info("Found {} entries in encoding manifest.", _encoding->count());

        _install = ResolveData<tact::data::Install>(_buildConfig->Install.Key.EncodingKey.ToString(),
            [&key = _buildConfig->Install.Key](io::IReadableStream& fstream) -> std::optional<tact::data::Install>
            {
                if (!fstream)
                    return std::nullopt;

                std::optional<tact::BLTE> compressedArchive = tact::BLTE::Parse(fstream, key.EncodingKey, key.ContentKey);
                if (!compressedArchive.has_value())
                    return std::nullopt;

                return tact::data::Install::Parse(compressedArchive->GetStream());
            }
        );

        if (!_install.has_value()) {
            _logger->error("An error occured while parsing install manifest.");
            return;
        }

        _logger->info("Found {} entries in install manifest.", _install->size());
    }

    std::optional<tact::data::FileLocation> LocalCache::FindFile(tact::CKey const& ckey) const {
        return _encoding->FindFile(ckey);
    }

    size_t LocalCache::GetContentKeySize() const {
        return _encoding->GetContentKeySize();
    }
}

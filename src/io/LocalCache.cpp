#include "io/LocalCache.hpp"
#include "tact/BLTE.hpp"

#include <spdlog/spdlog.h>

namespace io {
    LocalCache::LocalCache(std::filesystem::path installationRoot, net::ribbit::types::CDNs const& cdns, net::ribbit::types::versions::Record const& version, boost::asio::io_context& ctx)
        : _installRoot(installationRoot), _cdns(cdns), _version(version), _context(ctx)
    {
        _buildConfig = ResolveConfig<tact::config::BuildConfig>(_version.BuildConfig, [](io::IReadableStream& fstream) -> std::optional<tact::config::BuildConfig> {
            return tact::config::BuildConfig { fstream };
        });
        if (!_buildConfig.has_value())
            return;

        spdlog::info("Found build configuration {}.", _version.BuildConfig);

        _encoding = ResolveData<tact::data::Encoding>(_buildConfig->Encoding.Key.ContentKey, _buildConfig->Encoding.Key.EncodingKey,
            [&key= _buildConfig->Encoding.Key](io::IReadableStream& fstream) -> std::optional<tact::data::Encoding>
            {
                if (!fstream)
                    return std::nullopt;

                std::optional<tact::BLTE> compressedArchive = tact::BLTE::Parse(fstream, key.EncodingKey, key.ContentKey);
                return tact::data::Encoding{ };
            }
        );
    }
}
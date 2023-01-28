#include "io/LocalCache.hpp"
#include "tact/BLTE.hpp"

#include <spdlog/spdlog.h>

#include <fstream>

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
            [&key = _buildConfig->Encoding.Key](io::IReadableStream& fstream) -> std::optional<tact::data::Encoding>
            {
                if (!fstream)
                    return std::nullopt;

                std::optional<tact::BLTE> compressedArchive = tact::BLTE::Parse(fstream, key.EncodingKey, key.ContentKey);
                if (!compressedArchive.has_value())
                    return std::nullopt;

                spdlog::info("Loading encoding file {}...", key.EncodingKey.ToString());

                std::ofstream strm("test.encoding", std::ios::binary);
                std::vector<uint8_t> data; data.resize( compressedArchive->GetStream().GetLength());
                compressedArchive->GetStream().Read(data, std::endian::little);
                strm.write((char*) data.data(), data.size());

                compressedArchive->GetStream().SeekRead(0);

                return tact::data::Encoding { compressedArchive->GetStream() };
            }
        );

        if (_encoding.has_value())
            spdlog::info("Loaded {} entries from encoding.", _encoding->count());
    }
}

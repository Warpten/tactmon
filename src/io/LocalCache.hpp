#pragma once

#include "io/fs/MemoryMappedFileStream.hpp"
#include "io/net/Download.hpp"
#include "logging/Sinks.hpp"

#include "net/ribbit/types/CDNs.hpp"
#include "net/ribbit/types/Versions.hpp"

#include "tact/config/BuildConfig.hpp"
#include "tact/data/Encoding.hpp"
#include "tact/data/Install.hpp"
#include "tact/CKey.hpp"

#include <filesystem>
#include <functional>

#include <boost/asio/io_context.hpp>

namespace io {
    struct LocalCache final {
        LocalCache(std::filesystem::path installationRoot, net::ribbit::types::CDNs const& cdns, net::ribbit::types::versions::Record const& version, boost::asio::io_context& ctx);

    private:
        template <typename T>
        std::optional<T> ResolveConfig(std::string_view key, std::function<std::optional<T>(io::IReadableStream&)> parser) {
            for (net::ribbit::types::cdns::Record const& cdn : _cdns) {
                std::string remotePath{ std::format("{}/config/{}/{}/{}", cdn.Path, key.substr(0, 2), key.substr(2, 2), key) };
                std::filesystem::path localPath = _installRoot / remotePath;

                if (std::filesystem::is_regular_file(localPath)) {
                    io::MemoryMappedFileStream fileStream{ localPath, std::endian::little };
                    std::optional<T> parsedValue = parser(fileStream);
                    if (parsedValue.has_value())
                        return parsedValue;
                }

                for (std::string_view host : cdn.Hosts) {
                    if (!io::Download(_context, host, remotePath, localPath))
                        continue;

                    io::MemoryMappedFileStream fileStream { localPath, std::endian::little };

                    std::optional<T> parsedValue = parser(fileStream);
                    if (parsedValue.has_value())
                        return parsedValue;
                }
            }

            return std::nullopt;
        }

        template <typename T>
        std::optional<T> ResolveData(tact::CKey const& ckey, tact::EKey const& ekey, std::function<std::optional<T>(io::IReadableStream&)> parser) {
            std::string key = ekey.ToString();

            for (net::ribbit::types::cdns::Record const& cdn : _cdns) {
                std::string remotePath{ std::format("{}/data/{}/{}/{}", cdn.Path, key.substr(0, 2), key.substr(2, 2), key) };
                std::filesystem::path localPath = _installRoot / remotePath;

                if (std::filesystem::is_regular_file(localPath)) {
                    io::MemoryMappedFileStream fileStream { localPath, std::endian::little };
                    std::optional<T> parsedValue = parser(fileStream);
                    if (parsedValue.has_value())
                        return parsedValue;
                }

                for (std::string_view host : cdn.Hosts) {
                    if (!io::Download(_context, host, remotePath, localPath))
                        continue;

                    io::MemoryMappedFileStream fileStream{ localPath, std::endian::little };

                    std::optional<T> parsedValue = parser(fileStream);
                    if (parsedValue.has_value())
                        return parsedValue;
                }
            }

            return std::nullopt;
        }

    public:
        std::optional<tact::config::BuildConfig> const& GetBuildConfig() const { return _buildConfig; }

        std::optional<tact::data::FileLocation> FindFile(tact::CKey const& ckey) const;

    private:
        boost::asio::io_context& _context;
        std::shared_ptr<spdlog::logger> _logger;

    private:
        std::filesystem::path _installRoot;
        net::ribbit::types::CDNs _cdns;
        net::ribbit::types::versions::Record _version;

        std::optional<tact::config::BuildConfig> _buildConfig;
        std::optional<tact::data::Encoding> _encoding;
        std::optional<tact::data::Install> _install;
    };
}

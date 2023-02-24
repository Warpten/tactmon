#pragma once

#include "libtactmon/io/FileStream.hpp"
#include "libtactmon/net/DownloadTask.hpp"
#include "libtactmon/net/FileDownloadTask.hpp"
#include "libtactmon/ribbit/types/CDNs.hpp"
#include "libtactmon/ribbit/types/Versions.hpp"
#include "libtactmon/tact/BLTE.hpp"
#include "libtactmon/tact/Cache.hpp"
#include "libtactmon/tact/CKey.hpp"
#include "libtactmon/tact/config/BuildConfig.hpp"
#include "libtactmon/tact/config/CDNConfig.hpp"
#include "libtactmon/tact/data/Encoding.hpp"
#include "libtactmon/tact/data/FileLocation.hpp"
#include "libtactmon/tact/data/Index.hpp"
#include "libtactmon/tact/data/Install.hpp"

#include <cstdint>
#include <filesystem>
#include <memory>
#include <optional>
#include <string_view>

#include <boost/asio/io_context.hpp>

#include <fmt/format.h>

#include <spdlog/logger.h>

namespace libtactmon::tact::data::product {
    /**
     * An implementation of a game product.
     */
    struct Product {
        /**
         * Creates a new abstraction around a game product.
         * 
         * @param[in] productName The name of the product, as seen on Ribbit.
         * @param[in] localCache  A local cache manager controlling where configuration and data files will be read from and written to.
         */
        Product(std::string_view productName, Cache& localCache, boost::asio::io_context& context, std::shared_ptr<spdlog::logger> logger);

        /**
         * The name of this product.
         */
        std::string_view name() const { return _productName; }

    protected: // Resource resolution APIs

        /**
         * Resolves a configuration file.
         * 
         * @param[in] key    The MD5 hash of the config file.
         * @param[in] parser A function that parses the configuration file.
         * 
         * @returns The parsed configuration object, or an empty optional if an error occured.
         */
        template <typename T>
        std::optional<T> ResolveCachedConfig(std::string_view key, std::function<std::optional<T>(io::FileStream&)> parser) const {
            for (ribbit::types::cdns::Record const& cdn : *_cdns) {
                std::string relativePath { fmt::format("{}/config/{}/{}/{}", cdn.Path, key.substr(0, 2), key.substr(2, 2), key) };
                std::optional<T> cachedValue = _localCache.Resolve(relativePath, parser);
                if (cachedValue.has_value())
                    return cachedValue;

                for (std::string_view host : cdn.Hosts) {
                    net::FileDownloadTask downloadTask { relativePath, _localCache };
                    auto taskResult = downloadTask.Run(_context, host, _logger);
                    if (taskResult.has_value()) {
                        std::optional<T> parsedValue = parser(*taskResult);
                        if (parsedValue.has_value())
                            return parsedValue;
                    }
                }
            }

            return std::nullopt;
        }

        /**
         * Resolves a data file.
         * 
         * @param[in] taskSupplier   A function returning the task used to download said file.
         * @param[in] resultSupplier A function that parses the data file.
         * 
         * @returns The parsed data file, or an empty optional if an error occured.
         */
        template <typename Task, typename R>
        std::optional<R> ResolveData(std::function<Task()> taskSupplier,
            std::function<std::optional<R>(typename Task::ResultType&)> resultSupplier) const
        {
            Task downloadTask = taskSupplier();

            for (ribbit::types::cdns::Record const& cdn : *_cdns) {
                for (std::string_view host : cdn.Hosts) {
                    typename Task::ResultType taskResult = downloadTask.Run(_context, host, _logger);
                    if (taskResult.has_value())
                        return resultSupplier(taskResult);
                }
            }

            return std::nullopt;
        }

        /**
         * Resolves a cached file data.
         * 
         * @param[in] resourcePath   Path on disk to the resource.
         * @param[in] resultSupplier A function that deserializes the resource.
         * 
         * @returns An optional encapsulating the deserialized resource.
         */
        template <typename R>
        std::optional<R> ResolveCachedData(std::string_view key, std::function<std::optional<R>(io::FileStream&)> resultSupplier) const {
            for (ribbit::types::cdns::Record const& cdn : *_cdns) {
                std::string relativePath { fmt::format("{}/data/{}/{}/{}", cdn.Path, key.substr(0, 2), key.substr(2, 2), key) };
                std::optional<R> cachedValue = _localCache.Resolve<R>(relativePath, resultSupplier);
                if (cachedValue.has_value())
                    return cachedValue;

                net::FileDownloadTask fileTask { relativePath, _localCache };

                for (std::string_view host : cdn.Hosts) {
                    std::optional<io::FileStream> taskResult = fileTask.Run(_context, host, _logger);

                    if (taskResult.has_value())
                        return resultSupplier(*taskResult);
                }
            }

            return std::nullopt;
        }

        template <typename Task>
        std::optional<tact::BLTE> ResolveBLTE(std::function<Task()> taskSupplier) {
            return ResolveData(taskSupplier, [](typename Task::ValueType& result) {
                if (result.has_value())
                    return tact::BLTE::Parse(*result);

                return std::nullopt;
            });
        }

    public: // Front-facing API
        /**
         * Returns the version of this product that Ribbit exposes at the time this method is called.
         */
        std::optional<ribbit::types::Versions> Refresh() noexcept;

        /**
         * Loads the given configuration files.
         * 
         * @param[in] buildConfig The name of the build configuration file.
         * @param[in] cdnConfig   The name of the content domain network configuration file.
         */
        virtual bool Load(std::string_view buildConfig, std::string_view cdnConfig) noexcept;

        /**
         * Locates a file by its path.
         * 
         * @param[in] filePath Complete path to the file.
         * 
         * @returns The location of the file, or an empty optional if not found.
         */
        virtual std::optional<tact::data::FileLocation> FindFile(std::string_view filePath) const;

        /**
         * Locates a file by FDID.
         * 
         * @param[in] fileDataID The ID of the file in the product's Root file.
         * 
         * @returns The location of the file, or an empty optional if not found.
         */
        virtual std::optional<tact::data::FileLocation> FindFile(uint32_t fileDataID) const { return std::nullopt; }

        /**
         * Locates a file by content key.
         * 
         * @param[in] contentKey The content key of the file.
         * 
         * @returns The location of the file, or an empty optional if not found.
         */
        std::optional<tact::data::FileLocation> FindFile(tact::CKey const& contentKey) const;

        /**
         * Opens a file given its location.
         * 
         * @remarks This effectively downloads the file from the network, unless it just so happens to
         * have been cached in the case where it's not contained in an archive but is available as-is on
         * the CDN.
         * 
         * @param[in] location The file's location.
         * 
         * @returns A BLTE stream backed in memory, or an empty optional if the file could not be found.
         */
        std::optional<tact::BLTE> Open(tact::data::FileLocation const& location) const;

        /**
         * Locates the archive that encodes a given encoding key.
         * 
         * @param[in] ekey The encoding key.
         * @returns Location of the file in an archive, or an empty optional if the file could not be found.
         */
        std::optional<tact::data::ArchiveFileLocation> FindArchive(tact::EKey const& ekey) const;

    private:
        boost::asio::io_context& _context;
        std::string _productName;

    private:
        Cache& _localCache;

    protected:
        std::shared_ptr<spdlog::logger> _logger;

        std::optional<ribbit::types::CDNs> _cdns;

        std::optional<tact::config::BuildConfig> _buildConfig;
        std::optional<tact::config::CDNConfig> _cdnConfig;

        std::optional<tact::data::Encoding> _encoding;
        std::optional<tact::data::Install> _install;
        std::vector<tact::data::Index> _indices;
    };
}

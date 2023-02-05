#pragma once

#include "io/fs/FileStream.hpp"
#include "net/DownloadTask.hpp"
#include "net/ribbit/types/CDNs.hpp"
#include "net/ribbit/types/Versions.hpp"
#include "tact/BLTE.hpp"
#include "tact/CKey.hpp"
#include "tact/config/BuildConfig.hpp"
#include "tact/config/CDNConfig.hpp"
#include "tact/data/Encoding.hpp"
#include "tact/data/FileLocation.hpp"
#include "tact/data/Index.hpp"
#include "tact/data/Install.hpp"

#include <spdlog/logger.h>

#include <boost/asio/io_context.hpp>

#include <cstdint>
#include <filesystem>
#include <optional>
#include <string_view>

namespace tact::data::product {
    struct Product {
        Product(std::string_view productName, std::filesystem::path installationRoot,
            boost::asio::io_context& context);

    private: // Resource resolution APIs

        /**
        * Resolves a configuration file.
        * 
        * @param[in] key    The MD5 hash of the config file.
        * @param[in] parser A function that parses the configuration file.
        * 
        * @returns The parsed configuration object, or an empty optional if an error occured.
        */
        template <typename T>
        std::optional<T> ResolveConfig(std::string_view key, std::function<std::optional<T>(io::IReadableStream&)> parser) const {
            for (net::ribbit::types::cdns::Record const& cdn : *_cdns) {
                std::string remotePath{ std::format("{}/config/{}/{}/{}", cdn.Path, key.substr(0, 2), key.substr(2, 2), key) };
                std::filesystem::path localPath = _installRoot / remotePath;

                if (std::filesystem::is_regular_file(localPath)) {
                    io::FileStream fileStream { localPath, std::endian::little };
                    std::optional<T> parsedValue = parser(fileStream);
                    if (parsedValue.has_value())
                        return parsedValue;
                }

                for (std::string_view host : cdn.Hosts) {
                    net::FileDownloadTask downloadTask{ key, localPath };
                    auto taskResult = downloadTask.Run(_context, host, std::format("{}/config", cdn.Path), _logger);
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
        * @param[in] taskSupplier A function returning the task used to download said file.
        * @param[in] resultSupplier A function that parses the data file.
        * 
        * @returns The parsed data file, or an empty optional if an error occured.
        */
        template <typename Task, typename R>
        // requires std::is_base_of_v<net::DownloadTask, Task>
        std::optional<R> ResolveData(std::function<Task()> taskSupplier,
            std::function<std::optional<R>(typename Task::ResultType&)> resultSupplier) const
        {
            Task downloadTask = taskSupplier();

            for (net::ribbit::types::cdns::Record const& cdn : *_cdns) {
                std::string_view queryPath = cdn.Path;

                for (std::string_view host : cdn.Hosts) {
                    typename Task::ResultType taskResult = downloadTask.Run(_context,
                        host, std::format("{}/data", cdn.Path), _logger);
                    if (taskResult.has_value())
                        return resultSupplier(taskResult);
                }
            }

            return std::nullopt;
        }

        /**
        * Resolves a cached file data.
        * 
        * @param[in] resourcePath Path on disk to the resource.
        * @param[in] resultSupplier A function that deserializes the resource.
        * 
        * @returns An optional encapsulating the deserialized resource.
        */
        template <typename R>
        std::optional<R> ResolveCachedData(std::string_view key,
            std::function<std::optional<R>(io::FileStream&)> resultSupplier) const {

            namespace fs = std::filesystem;

            for (net::ribbit::types::cdns::Record const& cdn : *_cdns) {
                std::string remotePath{ std::format("{}/data/{}/{}/{}", cdn.Path, key.substr(0, 2), key.substr(2, 2), key) };
                std::filesystem::path localPath = _installRoot / remotePath;

                if (fs::is_regular_file(localPath)) {
                    io::FileStream fs { localPath, std::endian::little };
                    return resultSupplier(fs);
                }

                net::FileDownloadTask fileTask { key, localPath };

                std::string_view queryPath = cdn.Path;

                for (std::string_view host : cdn.Hosts) {
                    std::optional<io::FileStream> taskResult = fileTask.Run(_context,
                        host, std::format("{}/data", cdn.Path), _logger);

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
        virtual bool Refresh() noexcept;

        /**
         * Locates a file.
         * 
         * @param[in] fileName Complete path to the file.
         * 
         * @returns The location of the file, or an empty optional if not found.
         */
        virtual std::optional<tact::data::FileLocation> FindFile(std::string_view fileName) const;

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

    private:
        std::optional<tact::data::IndexFileLocation> FindIndex(tact::EKey const& ekey) const;

    private:
        boost::asio::io_context& _context;
        std::string _productName;

    private:
        std::filesystem::path _installRoot;

    protected:
        std::shared_ptr<spdlog::logger> _logger;

        std::optional<net::ribbit::types::CDNs> _cdns;
        std::optional<net::ribbit::types::versions::Record> _version;

        std::optional<tact::config::BuildConfig> _buildConfig;
        std::optional<tact::config::CDNConfig> _cdnConfig;

        std::optional<tact::data::Encoding> _encoding;
        std::optional<tact::data::Install> _install;
        std::vector<tact::data::Index> _indices;
    };
}

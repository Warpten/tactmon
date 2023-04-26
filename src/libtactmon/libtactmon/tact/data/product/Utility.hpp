#pragma once

#include "libtactmon/io/FileStream.hpp"
#include "libtactmon/net/FileDownloadTask.hpp"
#include "libtactmon/ribbit/types/CDNs.hpp"
#include "libtactmon/tact/Cache.hpp"
#include "libtactmon/Errors.hpp"
#include "libtactmon/Result.hpp"

#include <functional>
#include <memory>
#include <optional>
#include <string_view>

#include <boost/asio/any_io_executor.hpp>

#include <fmt/core.h>
#include <fmt/format.h>

#include <spdlog/logger.h>

namespace libtactmon::tact::data::product {
    /**
     * Exposes utility methods to resolve files from Blizzard CDNs.
     */
    struct ResourceResolver {
        ResourceResolver(boost::asio::any_io_executor executor, tact::Cache& localCache)
            : _executor(std::move(executor)), _localCache(localCache)
        { }

        /**
         * Resolves a configuration file.
         * 
         * @param[in] cdns   A list of available CDNs, as provided by Ribbit.
         * @param[in] key    The configuration file's key.
         * 
         * @returns A @ref Result holding a stream to the resource on disk, or an error value.
         */
        Result<io::FileStream> ResolveConfiguration(ribbit::types::CDNs const& cdns, std::string_view key) const {
            return Resolve(cdns, key, "/{}/config/{}/{}/{}");
        }

        /**
         * Resolves a data file.
         * 
         * @param[in] cdns   A list of available CDNs, as provided by Ribbit.
         * @param[in] key    The data file's key.
         * 
         * @returns A @ref Result holding a stream to the resource on disk, or an error value.
         */
        Result<io::FileStream> ResolveData(ribbit::types::CDNs const& cdns, std::string_view key) const {
            return Resolve(cdns, key, "/{}/data/{}/{}/{}");
        }

        /**
         * Resolves a block table-encoded data file.
         *
         * @param[in] cdns        A list of available CDNs, as provided by Ribbit.
         * @param[in] encodingKey The data file's encoding key.
         * @param[in] contentKey  The data file's content key.
         *
         * @returns A @ref Result holding a stream to the resource on disk, or an error value.
         */
        Result<tact::BLTE> ResolveBLTE(ribbit::types::CDNs const& cdns, tact::EKey const& encodingKey, tact::CKey const& contentKey) const {
            return ResolveData(cdns, encodingKey.ToString()).transform([&](io::FileStream compressedStream) {
                std::optional<tact::BLTE> decompressedStream = tact::BLTE::Parse(compressedStream, encodingKey, contentKey);
                if (decompressedStream.has_value())
                    return Result<tact::BLTE> { decompressedStream.value() };

                return Result<tact::BLTE> { Error::MalformedArchive };
            });
        }

        /**
         * Resolves a block table-encoded data file.
         *
         * @param[in] cdns        A list of available CDNs, as provided by Ribbit.
         * @param[in] encodingKey The data file's encoding key.
         *
         * @returns A @ref Result holding a stream to the resource on disk, or an error value.
         */
        Result<tact::BLTE> ResolveBLTE(ribbit::types::CDNs const& cdns, tact::EKey const& encodingKey) const {
            return ResolveData(cdns, encodingKey.ToString()).transform([&](io::FileStream compressedStream) {
                std::optional<tact::BLTE> decompressedStream = tact::BLTE::Parse(compressedStream);
                if (decompressedStream.has_value())
                    return Result<tact::BLTE> { decompressedStream.value() };

                return Result<tact::BLTE> { Error::MalformedArchive };
            });
        }

    private:
        Result<io::FileStream> Resolve(ribbit::types::CDNs const& cdns, std::string_view key, std::string_view formatString) const {
            for (ribbit::types::cdns::Record const& cdn : cdns) {
                std::string relativePath { fmt::format(fmt::runtime(formatString), cdn.Path, key.substr(0, 2), key.substr(2, 2), key) };

                auto cachedValue = _localCache.Resolve(relativePath);
                if (cachedValue.has_value())
                    return Result<io::FileStream> { std::move(cachedValue.value()) };

                for (std::string_view host : cdn.Hosts) {
                    net::FileDownloadTask downloadTask{ relativePath, _localCache };
                    auto taskResult = downloadTask.Run(_executor, host);
                    if (taskResult.has_value())
                        return taskResult;
                }
            }

            return Result<io::FileStream> { Error::ResourceResolutionFailed };
        }

    protected:
        boost::asio::any_io_executor _executor;

    public:
        tact::Cache& _localCache;
    };
}

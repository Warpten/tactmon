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
    class ResourceResolver {
        template <typename F>
        using ResultType = Result<std::invoke_result_t<F, io::FileStream&>>;

    public:
        ResourceResolver(boost::asio::any_io_executor executor, tact::Cache& localCache)
            : _executor(std::move(executor)), _localCache(localCache)
        { }

        /**
         * Resolves a configuration file.
         * 
         * @param[in] cdns   A list of available CDNs, as provided by Ribbit.
         * @param[in] key    The configuration file's key.
         * @param[in] parser A callable in charge of parsing the file.
         * 
         * @returns The parsed file or an empty optional if unable to.
         */
        template <typename Handler>
        auto ResolveConfiguration(ribbit::types::CDNs const& cdns, std::string_view key, Handler parser) const -> ResultType<Handler>
        {
            return Resolve(cdns, key, "/{}/config/{}/{}/{}", parser);
        }

        Result<io::FileStream> ResolveConfiguration(ribbit::types::CDNs const& cdns, std::string_view key) const {
            return Resolve(cdns, key, "/{}/config/{}/{}/{}");
        }

        /**
         * Resolves a data file.
         * 
         * @param[in] cdns   A list of available CDNs, as provided by Ribbit.
         * @param[in] key    The configuration file's key.
         * @param[in] parser A callable in charge of parsing the file.
         * 
         * @returns The parsed file or an empty optional if unable to.
         */
        template <typename Handler>
        auto ResolveData(ribbit::types::CDNs const& cdns, std::string_view key, Handler parser) const -> ResultType<Handler>
        {
            return Resolve(cdns, key, "/{}/data/{}/{}/{}", parser);
        }

        Result<io::FileStream> ResolveData(ribbit::types::CDNs const& cdns, std::string_view key) const {
            return Resolve(cdns, key, "/{}/data/{}/{}/{}");
        }

        Result<tact::BLTE> ResolveBLTE(ribbit::types::CDNs const& cdns, tact::EKey const& encodingKey, tact::CKey const& contentKey) const {
            return ResolveData(cdns, encodingKey.ToString()).transform([&](io::FileStream compressedStream) {
                std::optional<tact::BLTE> decompressedStream = tact::BLTE::Parse(compressedStream, encodingKey, contentKey);
                if (decompressedStream.has_value())
                    return Result<tact::BLTE> { decompressedStream.value() };

                return Result<tact::BLTE> { Error::MalformedArchive };
            });
        }

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

        template <typename Handler>
        auto Resolve(ribbit::types::CDNs const& cdns, std::string_view key, std::string_view formatString, Handler parser) const
            -> ResultType<Handler>
        {
            for (ribbit::types::cdns::Record const& cdn : cdns) {
                std::string relativePath { fmt::format(fmt::runtime(formatString), cdn.Path, key.substr(0, 2), key.substr(2, 2), key) };

                auto cachedValue = _localCache.Resolve(relativePath, parser);
                if (cachedValue.has_value())
                    return cachedValue;

                for (std::string_view host : cdn.Hosts) {
                    net::FileDownloadTask downloadTask { relativePath, _localCache };
                    auto taskResult = downloadTask.Run(_executor, host).and_then(parser);
                    if (taskResult.has_value())
                        return taskResult;
                }
            }

            return ResultType<Handler> { Error::ResourceResolutionFailed };
        }

    protected:
        boost::asio::any_io_executor _executor;

    public:
        tact::Cache& _localCache;
    };
}

#pragma once

#include "libtactmon/io/FileStream.hpp"
#include "libtactmon/net/FileDownloadTask.hpp"
#include "libtactmon/ribbit/types/CDNs.hpp"
#include "libtactmon/tact/Cache.hpp"

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
         * @param[in] parser A callable in charge of parsing the file.
         * @param[in] logger A logger for errors that occur during download.
         * 
         * @returns The parsed file or an empty optional if unable to.
         */
        template <typename Handler>
        auto ResolveConfiguration(ribbit::types::CDNs const& cdns,
            std::string_view key, Handler parser, spdlog::logger* logger = nullptr) const
            -> std::invoke_result_t<Handler, io::FileStream&>
        {
            return Resolve(cdns, key, "/{}/config/{}/{}/{}", parser, logger);
        }

        /**
         * Resolves a data file.
         * 
         * @param[in] cdns   A list of available CDNs, as provided by Ribbit.
         * @param[in] key    The configuration file's key.
         * @param[in] parser A callable in charge of parsing the file.
         * @param[in] logger A logger for errors that occur during download.
         * 
         * @returns The parsed file or an empty optional if unable to.
         */
        template <typename Handler>
        auto ResolveData(ribbit::types::CDNs const& cdns,
            std::string_view key, Handler parser, spdlog::logger* logger = nullptr) const
            -> std::invoke_result_t<Handler, io::FileStream&>
        {
            return Resolve(cdns, key, "/{}/data/{}/{}/{}", parser, logger);
        }

    private:
        template <typename Handler>
        auto Resolve(ribbit::types::CDNs const& cdns,
            std::string_view key, std::string_view formatString,
            Handler parser,
            spdlog::logger* logger = nullptr) const
            -> std::invoke_result_t<Handler, io::FileStream&>
        {
            for (ribbit::types::cdns::Record const& cdn : cdns) {
                std::string relativePath{ fmt::format(fmt::runtime(formatString), cdn.Path, key.substr(0, 2), key.substr(2, 2), key) };

                auto cachedValue = _localCache.Resolve(relativePath, parser);
                if (cachedValue.has_value())
                    return cachedValue;

                for (std::string_view host : cdn.Hosts) {
                    net::FileDownloadTask downloadTask{ relativePath, _localCache };
                    auto taskResult = downloadTask.Run(_executor, host, logger);
                    if (taskResult.has_value()) {
                        auto parsedValue = parser(*taskResult);

                        if (parsedValue.has_value())
                            return parsedValue;
                    }
                }
            }

            return std::nullopt;
        }

    protected:
        boost::asio::any_io_executor _executor;

    public:
        tact::Cache& _localCache;
    };
}

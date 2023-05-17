#pragma once

#include "libtactmon/io/FileStream.hpp"
#include "libtactmon/io/MemoryStream.hpp"
#include "libtactmon/ribbit/types/CDNs.hpp"
#include "libtactmon/tact/Cache.hpp"
#include "libtactmon/tact/CKey.hpp"
#include "libtactmon/tact/EKey.hpp"
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
            : _executor(executor), _localCache(localCache)
        { }

        /**
         * Resolves a configuration file.
         * 
         * @param[in] cdns   A list of available CDNs, as provided by Ribbit.
         * @param[in] key    The configuration file's key.
         * 
         * @returns A @ref Result holding a stream to the resource on disk, or an error value.
         */
        Result<io::FileStream> ResolveConfiguration(ribbit::types::CDNs const& cdns, std::string_view key) const;

        /**
         * Resolves a data file.
         * 
         * @param[in] cdns   A list of available CDNs, as provided by Ribbit.
         * @param[in] key    The data file's key.
         * 
         * @returns A @ref Result holding a stream to the resource on disk, or an error value.
         */
        Result<io::FileStream> ResolveData(ribbit::types::CDNs const& cdns, std::string_view key) const;

        /**
         * Resolves a block table-encoded data file.
         *
         * @param[in] cdns        A list of available CDNs, as provided by Ribbit.
         * @param[in] encodingKey The data file's encoding key.
         * @param[in] contentKey  The data file's content key.
         *
         * @returns A @ref Result holding a stream to the resource on disk, or an error value.
         */
        Result<io::GrowableMemoryStream> ResolveBLTE(ribbit::types::CDNs const& cdns, tact::EKey const& encodingKey, tact::CKey const& contentKey) const;

        /**
         * Resolves a block table-encoded data file.
         *
         * @param[in] cdns        A list of available CDNs, as provided by Ribbit.
         * @param[in] encodingKey The data file's encoding key.
         *
         * @returns A @ref Result holding a stream to the resource on disk, or an error value.
         */
        Result<io::GrowableMemoryStream> ResolveBLTE(ribbit::types::CDNs const& cdns, tact::EKey const& encodingKey) const;

    protected:
        boost::asio::any_io_executor _executor;

    public:
        tact::Cache& _localCache;
    };
}

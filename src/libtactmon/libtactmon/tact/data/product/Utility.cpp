#include "libtactmon/io/BlockTableEncodedStreamTransform.hpp"
#include "libtactmon/net/FileDownloadTask.hpp"
#include "libtactmon/tact/data/product/Utility.hpp"

namespace libtactmon::tact::data::product {
    static io::BlockTableEncodedStreamTransform transform;

    namespace detail {
        template <typename T> struct is_result : std::false_type { };
        template <typename R, typename E> struct is_result<Result<R, E>> : std::true_type { };
    }

    template <typename R, typename F>
    auto Resolve(boost::asio::any_io_executor const& executor, tact::Cache& localCache, ribbit::types::CDNs const& cdns, std::string_view key,
        std::string_view formatString, F atomicOperation) -> std::invoke_result_t<F, io::FileStream>
    {
        for (ribbit::types::cdns::Record const& cdn : cdns) {
            std::string relativePath{ fmt::format(fmt::runtime(formatString), cdn.Path, key.substr(0, 2), key.substr(2, 2), key) };

            auto cachedValue = localCache.Resolve(relativePath).transform(atomicOperation);
            if (cachedValue.has_value())
                return cachedValue;

            for (std::string_view host : cdn.Hosts) {
                net::FileDownloadTask downloadTask{ relativePath, localCache };
                auto taskResult = downloadTask.Run(executor, host).transform(atomicOperation);
                if (taskResult.has_value())
                    return taskResult;
            }
        }

        return Result<R> { Error::ResourceResolutionFailed };
    }

    Result<io::FileStream> ResourceResolver::ResolveConfiguration(ribbit::types::CDNs const& cdns, std::string_view key) const {
        return Resolve<io::FileStream>(_executor, _localCache, cdns, key, "/{}/config/{}/{}/{}", [](io::FileStream fs) {
            return Result<io::FileStream> { std::move(fs) };
        });
    }

    Result<io::FileStream> ResourceResolver::ResolveData(ribbit::types::CDNs const& cdns, std::string_view key) const {
        return Resolve<io::FileStream>(_executor, _localCache, cdns, key, "/{}/data/{}/{}/{}", [](io::FileStream fs) {
            return Result<io::FileStream> { std::move(fs) };
        });
    }

    Result<io::GrowableMemoryStream> ResourceResolver::ResolveBLTE(ribbit::types::CDNs const& cdns, tact::EKey const& encodingKey, tact::CKey const& contentKey) const {
        return Resolve<io::GrowableMemoryStream>(_executor, _localCache, cdns, encodingKey.ToString(), "/{}/data/{}/{}/{}", [&](io::FileStream fs) {
            return transform(fs, std::addressof(encodingKey), std::addressof(contentKey));
        });
    }

    Result<io::GrowableMemoryStream> ResourceResolver::ResolveBLTE(ribbit::types::CDNs const& cdns, tact::EKey const& encodingKey) const {
        return Resolve<io::GrowableMemoryStream>(_executor, _localCache, cdns, encodingKey.ToString(), "/{}/data/{}/{}/{}", [&](io::FileStream fs) {
            return transform(fs, std::addressof(encodingKey), nullptr);
        });
    }
}

#pragma once

#include "libtactmon/detail/Export.hpp"

#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

#include <boost/beast/http/status.hpp>
#include <boost/system/error_code.hpp>

#include <fmt/format.h>

namespace libtactmon {
    namespace tact {
        struct EKey;
    }

    namespace errors {
        enum class Category : uint8_t {
            None = 0,
            Network,
            BLTE,
            FileSystem,
            Config,
            Ribbit,
            Product
        };

        enum class Code : uint32_t {
            OK = 0, // Sigh.

            ResourceResolutionFailed,

            // BLTE
            MalformedArchive,
            InvalidSignature,
            ChunkDecompressionFailure,
            UnsupportedCompressionMode,

            // Network
            NetworkError,
            LocalInitializationFailed,
            BadStatusCode,
            ConnectError,
            ReadError,
            WriteError,

            // Filesystem
            FileNotFound,

            // Config,
            MalformedFile,
            InvalidPropertySpecification,
            InvalidContentKey,
            InvalidEncodingKey,

            // Ribbit,
            MalformedMultipartMessage,
            Unparsable,

            // Product
            InvalidIndexFile,
            InvalidInstallFile,
            InvalidRootFile,
            RootNotFound,
        };

        struct LIBTACTMON_API Error final {
            Error(Category category, Code code, std::string what) noexcept : _what(std::move(what)), _category(category), _code(code) { }

            template <typename... Ts, typename = std::enable_if_t<(sizeof...(Ts) > 0)>>
            Error(Category category, Code code, std::string_view fmt, Ts&&... args) noexcept : _what(fmt::format(fmt::runtime(fmt), std::forward<Ts>(args)...)), _category(category), _code(code) { }

        public:
            std::string_view what() const { return _what; }

            Code code() const { return _code; }
            Category category() const { return _category; }

        private:
            std::string _what;
            Category _category;
            Code _code;
        };

        inline bool operator == (Error const& left, Error const& right) {
            return left.code() == right.code();
        }
        inline bool operator != (Error const& left, Error const& right) {
            return !(left == right);
        }

        extern Error Success;

        extern Error ResourceResolutionFailed(std::string_view resourceName, std::string_view reason);

        namespace blte {
            extern Error MalformedArchive(libtactmon::tact::EKey const* encodingKey);
            extern Error InvalidSignature(libtactmon::tact::EKey const* encodingKey);
            extern Error ChunkDecompressionFailure(libtactmon::tact::EKey const* encodingKey, std::size_t chunkIndex, std::size_t archiveOffset);
            extern Error UnsupportedCompressionMode(libtactmon::tact::EKey const* encodingKey, std::size_t chunkIndex, uint8_t compressionMode, std::size_t archiveOffset);
        }

        namespace network {
            extern Error ConnectError(boost::system::error_code const& ec, std::string_view host);
            extern Error ReadError(boost::system::error_code const& ec, std::string_view host);
            extern Error WriteError(boost::system::error_code const& ec, std::string_view host);

            extern Error NetworkError(std::string_view resourcePath, std::string_view resourceHost, boost::system::error_code const& ec);
            extern Error LocalInitializationFailed(std::string_view resourcePath, std::string_view resourceHost, boost::system::error_code const& ec);
            extern Error BadStatusCode(std::string_view resourcePath, std::string_view resourceHost, boost::beast::http::status statusCode);
            extern Error ReadError(std::string_view resourcePath, std::string_view resourceHost, boost::system::error_code const& ec);
        }

        namespace fs {
            extern Error FileNotFound(std::string_view resourcePath);
        }

        // TODO: Make these expose the name of the failing file.
        namespace cfg {
            extern Error MalformedFile(std::string_view name);
            extern Error InvalidPropertySpecification(std::string_view propertyName, std::vector<std::string_view> tokens);
            extern Error InvalidContentKey(std::string_view token);
            extern Error InvalidEncodingKey(std::string_view token);
        }

        namespace ribbit {
            extern Error MalformedFile(std::string_view command);
            extern Error MalformedMultipartMessage(std::string_view command);
            extern Error Unparsable(std::string_view command);
        }

        namespace tact {
            extern Error InvalidIndexFile(std::string_view hash, std::string_view reason);
            extern Error InvalidInstallFile(std::string_view hash, std::string_view reason);
            extern Error InvalidRootFile(std::string_view hash, std::string_view reason);

            extern Error RootNotFound();
        }
    }
}

template <>
struct LIBTACTMON_API fmt::formatter<libtactmon::errors::Error> {
    template <typename ParseContext>
    constexpr auto parse(ParseContext& ctx) {
        return ctx.begin();
    }

    template <typename FormatContext>
    auto format(libtactmon::errors::Error const& error, FormatContext& ctx) {
        return fmt::format_to(ctx.out(), "{}", error.what());
    }
};

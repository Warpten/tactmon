#include "libtactmon/tact/CKey.hpp"
#include "libtactmon/tact/EKey.hpp"
#include "libtactmon/Errors.hpp"

#include <fmt/ostream.h>

#include <string>

namespace libtactmon::errors {
    Error Success{ Category::None, Code::OK, "" };

    Error ResourceResolutionFailed(std::string_view resourceName, std::string_view reason) {
        return Error { Category::Network, Code::ResourceResolutionFailed, "An error occured while retrieving {}: {}", resourceName, reason };
    }

    namespace blte {
        Error MalformedArchive(libtactmon::tact::EKey const* encodingKey) {
            if (encodingKey != nullptr)
                return Error { Category::BLTE, Code::MalformedArchive, "Archive {} is malformed", encodingKey->ToString() };

            return Error { Category::BLTE, Code::MalformedArchive, "Archive is malformed" };
        }
        Error InvalidSignature(libtactmon::tact::EKey const* encodingKey) {
            if (encodingKey != nullptr)
                return Error { Category::BLTE, Code::InvalidSignature, "Archive {} does not match its encoding key", encodingKey->ToString() };

            return Error { Category::BLTE, Code::InvalidSignature, "Archive does not match its encoding key" };
        }
        Error ChunkDecompressionFailure(libtactmon::tact::EKey const* encodingKey, std::size_t chunkIndex, std::size_t archiveOffset) {
            if (encodingKey != nullptr)
                return Error { Category::BLTE, Code::ChunkDecompressionFailure, "Decompression of chunk {} of archive {} (at offset {}) failed", chunkIndex, encodingKey->ToString(), archiveOffset };

            return Error { Category::BLTE, Code::ChunkDecompressionFailure, "Decompression of chunk {} of archive ?? (at offset {}) failed", chunkIndex, archiveOffset };
        }
        Error UnsupportedCompressionMode(libtactmon::tact::EKey const* encodingKey, std::size_t chunkIndex, uint8_t compressionMode, std::size_t archiveOffset) {
            if (encodingKey != nullptr)
                return Error { Category::BLTE, Code::UnsupportedCompressionMode, "Chunk {} of archive {} (at offset {}) uses unknown compression mode {:X}", chunkIndex, encodingKey->ToString(), archiveOffset, compressionMode };

            return Error { Category::BLTE, Code::UnsupportedCompressionMode, "Chunk {} of archive ?? (at offset {}) uses unknown compression mode {:X}", chunkIndex, archiveOffset, compressionMode };
        }
    }

    namespace network {
        Error ConnectError(boost::system::error_code const& ec, std::string_view host) {
            return Error { Category::Network, Code::ConnectError, "An error occured while connecting to {}: {}", host, ec.message() };
        }
        Error ReadError(boost::system::error_code const& ec, std::string_view host) {
            return Error { Category::Network, Code::ReadError, "An error occured while reading from {}: {}", host, ec.message() };
        }
        extern Error WriteError(boost::system::error_code const& ec, std::string_view host) {
            return Error { Category::Network, Code::WriteError, "An error occured while writing to {}: {}", host, ec.message() };
        }

        Error NetworkError(std::string_view resourcePath, std::string_view resourceHost, boost::system::error_code const& ec) {
            return Error { Category::Network, Code::NetworkError, "An error occured while downloading {} from {}: {}", resourcePath, resourceHost, ec.message() };
        }
        Error LocalInitializationFailed(std::string_view resourcePath, std::string_view resourceHost, boost::system::error_code const& ec) {
            return Error { Category::Network, Code::LocalInitializationFailed, "An error occured while creating a local resource for {} from {}: {}",
                resourcePath, resourceHost, ec.message() };
        }
        Error BadStatusCode(std::string_view resourcePath, std::string_view resourceHost, boost::beast::http::status statusCode) {
            namespace http = boost::beast::http;

            return Error { Category::Network, Code::BadStatusCode, "An error occured while downloading {} from {}: Received HTTP status code {} {}",
                resourcePath, resourceHost, static_cast<unsigned>(statusCode), http::obsolete_reason(statusCode) };
        }
        Error ReadError(std::string_view resourcePath, std::string_view resourceHost, boost::system::error_code const& ec) {
            return Error { Category::Network, Code::ReadError, "An error occured while downloading {} from {}: {}", resourcePath, resourceHost, ec.message() };
        }
    }

    namespace fs {
        Error FileNotFound(std::string_view resourcePath) {
            return Error { Category::FileSystem, Code::FileNotFound, "Could not find {} in local cache", resourcePath };
        }
    }

    namespace cfg {
        Error MalformedFile(std::string_view name) {
            return Error { Category::Config, Code::MalformedFile, "Configuration file {} is malformed", name };
        }
        Error InvalidPropertySpecification(std::string_view propertyName, std::vector<std::string_view> tokens) {
            return Error { Category::Config, Code::InvalidPropertySpecification, "Could not process property {} with value '{}'", propertyName, fmt::join(tokens, " ")};
        }
        Error InvalidContentKey(std::string_view token) {
            return Error { Category::Config, Code::InvalidContentKey, "Invalid content key '{}'", token };
        }
        Error InvalidEncodingKey(std::string_view token) {
            return Error{ Category::Config, Code::InvalidEncodingKey, "Invalid encoding key '{}'", token };
        }
    }

    namespace tact {
        Error InvalidIndexFile(std::string_view hash, std::string_view reason) {
            return Error { Category::Product, Code::InvalidIndexFile, "An error occured while parsing index file {}.index: {}", hash, reason };
        }
        Error InvalidInstallFile(std::string_view hash, std::string_view reason) {
            return Error { Category::Product, Code::InvalidInstallFile, "An error occured while parsing install file {}: {}", hash, reason };
        }
        Error InvalidRootFile(std::string_view hash, std::string_view reason) {
            return Error { Category::Product, Code::InvalidRootFile, "An error occured while parsing root file {}: {}", hash, reason };
        }
        Error RootNotFound() {
            return Error { Category::Product, Code::RootNotFound, "Root manifest could not be found" };
        }
    }
}

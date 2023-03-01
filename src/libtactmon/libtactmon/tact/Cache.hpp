#pragma once

#include "libtactmon/io/FileStream.hpp"

#include <filesystem>
#include <functional>
#include <optional>
#include <string_view>

namespace libtactmon::tact {
    /**
     * Represents a portion of the local filesystem that stores copies of files from Blizzard CDNs.
     */
    struct Cache final {
        explicit Cache(std::filesystem::path root);

        /**
         * Resolves a resource on disk.
         */
        template <typename T>
        std::optional<T> Resolve(std::string_view resourcePath, std::function<std::optional<T>(io::FileStream&)> handler) {
            std::filesystem::path fullResourcePath = _root;

            // Avoid reverting to absolute paths
            // Maybe a code smell ?
            if (resourcePath.size() > 0 && (resourcePath[0] == '/' || resourcePath[0] == '\\'))
                fullResourcePath /= resourcePath.substr(1);

            if (!std::filesystem::is_regular_file(fullResourcePath))
                return std::nullopt;

            io::FileStream fstream { fullResourcePath, std::endian::little };
            return handler(fstream);
        }

        /**
         * Resolves the absolute path to the specific resource. Does not check for the resource's existence!
         */
        std::filesystem::path GetAbsolutePath(std::string_view relativePath) const { return _root / relativePath; }

        /**
         * Returns a stream around a file, allowing to write to it. If the file does not exist, it is created before this function returns.
         */
        io::FileStream OpenWrite(std::string_view relativePath) const;

        /**
         * Deletes a file from the cache.
         */
        void Delete(std::string_view relativePath) const;

    private:
        std::filesystem::path _root;
    };
}

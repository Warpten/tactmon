#pragma once

#include "libtactmon/io/FileStream.hpp"
#include "libtactmon/utility/FunctionRef.hpp"

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
         *
         * @param[in] resourcePath Relative path to the resource.
         * @param[in] handler      A Callable that will attempt to parse the resource.
         */
        template <typename T, typename Handler>
        requires std::is_same_v<std::optional<T>, std::invoke_result_t<Handler, io::FileStream&>>
        std::optional<T> Resolve(std::string_view resourcePath, Handler handler) {
            std::filesystem::path fullResourcePath = GetAbsolutePath(resourcePath);

            if (!std::filesystem::is_regular_file(fullResourcePath))
                return std::nullopt;

            try {
                io::FileStream fstream { fullResourcePath };
                return handler(fstream);
            } catch (std::exception const& ex) {
                // TODO: possibly log this
                Delete(resourcePath);
                return std::nullopt;
            }
        }

        /**
         * Resolves the absolute path to the specific resource. Does not check for the resource's existence!
         */
        std::filesystem::path GetAbsolutePath(std::string_view relativePath) const;

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

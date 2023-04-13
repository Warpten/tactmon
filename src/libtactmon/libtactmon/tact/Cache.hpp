#pragma once

#include "libtactmon/detail/Export.hpp"
#include "libtactmon/io/FileStream.hpp"

#include <filesystem>
#include <functional>
#include <optional>
#include <string_view>

namespace libtactmon::tact {
    /**
     * Represents a portion of the local filesystem that stores copies of files from Blizzard CDNs.
     */
    struct LIBTACTMON_API Cache final {
        explicit Cache(const std::filesystem::path& root);

        /**
         * Resolves a resource on disk.
         *
         * @param[in] resourcePath Relative path to the resource.
         * @param[in] handler      A Callable that will attempt to parse the resource.
         */
        template <typename Handler>
        auto Resolve(std::string_view resourcePath, Handler handler)
            -> std::invoke_result_t<Handler, io::FileStream&>
        {
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
        [[nodiscard]] std::filesystem::path GetAbsolutePath(std::string_view relativePath) const;

        /**
         * Returns a stream around a file, allowing to write to it. If the file does not exist, it is created before this function returns.
         */
        [[nodiscard]] io::FileStream OpenWrite(std::string_view relativePath) const;

        /**
         * Deletes a file from the cache.
         */
        void Delete(std::string_view relativePath) const;

    private:
        std::filesystem::path _root;
    };
}

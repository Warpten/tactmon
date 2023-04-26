#pragma once

#include "libtactmon/detail/Export.hpp"
#include "libtactmon/io/FileStream.hpp"
#include "libtactmon/Result.hpp"

#include <filesystem>
#include <functional>
#include <optional>
#include <string_view>

namespace libtactmon::tact {
    /**
     * Represents a portion of the local filesystem that stores copies of files from Blizzard CDNs.
     */
    struct LIBTACTMON_API Cache final {
        /**
         * Construct a cache with its root located at a provided path on disk.
         *
         * @param[in] root Root of the cache on disk.
         */
        explicit Cache(const std::filesystem::path& root);

        /**
         * Resolves a resource on disk.
         *
         * @param[in] resourcePath Relative path to the resource.
         * @param[in] handler      A @p Callable that will attempt to parse the resource.
         * @returns The result of the @p Callable.
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
                Delete(resourcePath);

                return std::nullopt;
            }
        }

        std::optional<io::FileStream> Resolve(std::string_view resourcePath) {
            std::filesystem::path fullResourcePath = GetAbsolutePath(resourcePath);

            if (!std::filesystem::is_regular_file(fullResourcePath))
                return std::nullopt;

            try {
                return io::FileStream { fullResourcePath };
            } catch (std::exception const& ex) {
                Delete(resourcePath);

                return std::nullopt;
            }
        }

        /**
         * Resolves the absolute path to the specific resource. Does not check for the resource's existence!
         *
         * @param[in] relativePath Relative path to the file.
         * @returns An absolute path on disk to the file.
         */
        [[nodiscard]] std::filesystem::path GetAbsolutePath(std::string_view relativePath) const;

        /**
         * Returns a stream around a file, allowing to write to it.
         * If the file does not exist, it is created before this function returns.
         *
         * @param[in] relativePath Relative path to the file.
         * @returns A writable stream.
         */
        [[nodiscard]] Result<io::FileStream> OpenWrite(std::string_view relativePath) const;

        /**
         * Deletes a file from the cache.
         * If the file does not exist, this function does nothing.
         *
         * @param[in] relativePath Relative path to the file.
         */
        void Delete(std::string_view relativePath) const;

    private:
        std::filesystem::path _root;
    };
}

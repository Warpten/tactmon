#pragma once

#include "io/fs/FileStream.hpp"
#include "io/IStream.hpp"

#include <filesystem>
#include <functional>
#include <optional>
#include <string_view>

namespace tact {
    struct Cache final {
        explicit Cache(std::filesystem::path root);

        /**
         * Resolves a resource on disk.
         */
        template <typename T>
        std::optional<T> Resolve(std::string_view resourcePath, std::function<std::optional<T>(io::FileStream&)> handler) {
            std::filesystem::path fullResourcePath = _root / resourcePath;
            if (!std::filesystem::is_regular_file(fullResourcePath))
                return std::nullopt;

            io::FileStream fstream { fullResourcePath, std::endian::little };
            return handler(fstream);
        }

        std::filesystem::path GetAbsolutePath(std::string_view relativePath) const { return _root / relativePath; }

        io::FileStream OpenWrite(std::string_view relativePath) const;

        void Delete(std::string_view relativePath) const;

    private:
        std::filesystem::path _root;
    };
}

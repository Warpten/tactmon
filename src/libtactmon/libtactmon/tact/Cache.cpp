#include "libtactmon/tact/Cache.hpp"

namespace libtactmon::tact {
    Cache::Cache(const std::filesystem::path& root) : _root(root) {
        if (!std::filesystem::is_directory(root))
            std::filesystem::create_directories(root);
    }

    std::filesystem::path Cache::GetAbsolutePath(std::string_view relativePath) const { 
        std::filesystem::path fullResourcePath = _root;

        // Avoid reverting to absolute paths
        // Maybe a code smell ?
        if (!relativePath.empty() && (relativePath[0] == '/' || relativePath[0] == '\\'))
            return fullResourcePath / relativePath.substr(1);

        return _root / relativePath;
    }

    Result<io::FileStream> Cache::OpenWrite(std::string_view relativePath) const {
        std::filesystem::path absolutePath = GetAbsolutePath(relativePath);
        if (!std::filesystem::is_directory(absolutePath.parent_path()))
            std::filesystem::create_directories(absolutePath.parent_path());

        return Result<io::FileStream> { absolutePath };
    }

    void Cache::Delete(std::string_view relativePath) const {
        std::filesystem::remove(GetAbsolutePath(relativePath));
    }
}

#include "libtactmon/tact/Cache.hpp"

namespace libtactmon::tact {
    Cache::Cache(std::filesystem::path root) : _root(root) { 
        if (!std::filesystem::is_directory(root))
            std::filesystem::create_directories(root);
    }

    io::FileStream Cache::OpenWrite(std::string_view relativePath) const {
        std::filesystem::path absolutePath = GetAbsolutePath(relativePath);
        if (!std::filesystem::is_directory(absolutePath.parent_path()))
            std::filesystem::create_directories(absolutePath.parent_path());

        return io::FileStream { absolutePath, std::endian::little };
    }

    void Cache::Delete(std::string_view relativePath) const {
        std::filesystem::remove(GetAbsolutePath(relativePath));
    }
}

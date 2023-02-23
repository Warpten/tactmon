#include "libtactmon/tact/Cache.hpp"

namespace libtactmon::tact {
    Cache::Cache(std::filesystem::path root) : _root(root) { }

    io::FileStream Cache::OpenWrite(std::string_view relativePath) const {
        return io::FileStream { GetAbsolutePath(relativePath), std::endian::little };
    }

    void Cache::Delete(std::string_view relativePath) const {
        std::filesystem::remove(GetAbsolutePath(relativePath));
    }
}

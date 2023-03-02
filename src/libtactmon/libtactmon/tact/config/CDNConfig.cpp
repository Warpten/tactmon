#include "libtactmon/detail/Tokenizer.hpp"
#include "libtactmon/tact/config/CDNConfig.hpp"
#include "libtactmon/io/IReadableStream.hpp"

#include <charconv>
#include <functional>
#include <string_view>
#include <vector>

using namespace std::string_view_literals;

namespace libtactmon::tact::config {
    std::optional<CDNConfig> CDNConfig::Parse(io::IReadableStream& stream) {
        stream.SeekRead(0);

        std::string_view contents = reinterpret_cast<const char*>(stream.Data());
        std::vector<std::string_view> lines = detail::Tokenize(contents, '\n');

        if (lines.empty())
            return std::nullopt;

        CDNConfig config;

        std::optional<CDNConfig::Archive> fileIndex = std::nullopt;

        for (std::string_view line : lines) {
            if (line.empty() || line[0] == '#')
                continue;

            static const char Separators[] = { ' ', '=' };
            std::vector<std::string_view> tokens = detail::Tokenize(line, std::span{ Separators }, true);

            if (tokens[0] == "archives") {
                config._archives.resize(tokens.size() - 1);
                for (size_t i = 1; i < tokens.size(); ++i)
                    config._archives[i - 1].Name = tokens[i];
            }
            else if (tokens[0] == "archives-index-size") {
                for (size_t i = 1; i < tokens.size(); ++i) {
                    auto [ptr, ec] = std::from_chars(tokens[i].data(), tokens[i].data() + tokens[i].size(), config._archives[i - 1].Size);
                    if (ec != std::errc{ })
                        return std::nullopt;
                }
            }
            else if (tokens[0] == "file-index") {
                if (tokens.size() == 1)
                    return std::nullopt;

                if (!fileIndex.has_value())
                    fileIndex.emplace();
                fileIndex->Name = tokens[1];
            }
            else if (tokens[1] == "file-index-size") {
                if (tokens.size() == 1)
                    return std::nullopt;
                if (!fileIndex.has_value())
                    fileIndex.emplace();
                auto [ptr, ec] = std::from_chars(tokens[1].data(), tokens[1].data() + tokens[1].size(), fileIndex->Size);
                if (ec != std::errc{ })
                    return std::nullopt;
            }
        }

        if (fileIndex.has_value())
            config._archives.emplace_back(*fileIndex);

        config._archives.shrink_to_fit();

        return config;
    }

    void CDNConfig::ForEachArchive(std::function<void(std::string_view, size_t)> handler) {
        for (Archive const& archive : _archives)
            handler(archive.Name, archive.Size);
    }
}

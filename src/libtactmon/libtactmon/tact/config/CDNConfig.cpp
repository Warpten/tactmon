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

        std::string_view contents { stream.Data<char>().data(), stream.GetLength() };
        std::vector<std::string_view> lines = detail::Tokenize(contents, '\n');

        if (lines.empty())
            return std::nullopt;

        CDNConfig config;
        for (std::string_view line : lines) {
            if (line.empty() || line[0] == '#')
                continue;

            static const char Separators[] = { ' ', '=' };
            std::vector<std::string_view> tokens = detail::Tokenize(line, std::span { Separators }, true);

            if (tokens[0] == "archives") {
                config.archives.resize(tokens.size() - 1);
                for (size_t i = 1; i < tokens.size(); ++i)
                    config.archives[i - 1].Name = tokens[i];
            }
            else if (tokens[0] == "archives-index-size") {
                for (size_t i = 1; i < tokens.size(); ++i) {
                    auto [ptr, ec] = std::from_chars(tokens[i].data(), tokens[i].data() + tokens[i].size(), config.archives[i - 1].Size);
                    if (ec != std::errc{ })
                        return std::nullopt;
                }
            }
            else if (tokens[0] == "file-index") {
                if (tokens.size() == 1)
                    return std::nullopt;

                if (!config.fileIndex.has_value())
                    config.fileIndex.emplace();

                config.fileIndex->Name = tokens[1];
            }
            else if (tokens[0] == "file-index-size") {
                if (tokens.size() == 1)
                    return std::nullopt;

                if (!config.fileIndex.has_value())
                    config.fileIndex.emplace();

                auto [ptr, ec] = std::from_chars(tokens[1].data(), tokens[1].data() + tokens[1].size(), config.fileIndex->Size);
                if (ec != std::errc{ })
                    return std::nullopt;
            }
        }

        return config;
    }
}

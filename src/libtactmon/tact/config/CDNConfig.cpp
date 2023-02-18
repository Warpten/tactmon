#include "tact/config/CDNConfig.hpp"
#include "ext/Tokenizer.hpp"
#include "io/IStream.hpp"

#include <charconv>
#include <functional>
#include <string_view>
#include <vector>

using namespace std::string_view_literals;

namespace tact::config {
    CDNConfig::CDNConfig(io::IReadableStream& fileStream) {
        fileStream.SeekRead(0);
        std::string fileContents;
        fileStream.ReadString(fileContents, fileStream.GetLength());

        std::vector<std::string_view> lines = ext::Tokenize(std::string_view{ fileContents }, '\n');

        for (std::string_view line : lines) {
            if (line.empty() || line[0] == '#')
                continue;

            static const char Separators[] = { ' ', '=' };
            std::vector<std::string_view> tokens = ext::Tokenize(line, std::span{ Separators }, true);

            if (tokens[0] == "archives") {
                for (size_t i = 1; i < tokens.size(); ++i)
                    _archives.emplace_back(tokens[i]);
            } else if (tokens[0] == "archives-index-size") {
                for (size_t i = 1; i < tokens.size(); ++i) {
                    auto [ptr, ec] = std::from_chars(tokens[i].data(), tokens[i].data() + tokens[i].size(), _archives[i - 1].Size);
                    if (ec != std::errc{ })
                        _archives[i - 1].Size = 0;
                }
            }

            // TODO: Process everything else?
        }
    }

    void CDNConfig::ForEachArchive(std::function<void(std::string_view, size_t)> handler) {
        for (Archive const& archive : _archives)
            handler(archive.Name, archive.Size);
    }
}

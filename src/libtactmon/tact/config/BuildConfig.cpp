#include "tact/config/BuildConfig.hpp"
#include "ext/Tokenizer.hpp"
#include "io/IStream.hpp"

#include <charconv>
#include <functional>
#include <string_view>
#include <vector>

// #include <boost/spirit/include/qi.hpp>

using namespace std::string_view_literals;

// namespace qi = boost::spirit::qi;
// namespace ascii = boost::spirit::ascii;

namespace tact::config {
    struct ConfigHandler {
        std::string_view Token;

        std::function<void(BuildConfig&, std::vector<std::string_view>)> Handler;
    };

    static const ConfigHandler Handlers[] = {
        { "root", 
            [](BuildConfig& cfg, auto tokens) {
                cfg.Root = CKey { tokens[1] };
            }
        }, { "install",
            [](BuildConfig& cfg, auto tokens) {
                cfg.Install.Key.ContentKey = CKey { tokens[1] };
                if (tokens.size() == 3)
                    cfg.Install.Key.EncodingKey = EKey { tokens[2] };
            }
        }, { "install-size",
            [](BuildConfig& cfg, auto tokens) {
                std::from_chars(tokens[1].data(), tokens[1].data() + tokens[1].size(), cfg.Install.Size[0]);
                if (tokens.size() == 3)
                    std::from_chars(tokens[2].data(), tokens[2].data() + tokens[2].size(), cfg.Install.Size[1]);
            }
        },
        // download
        // download-size
        // size
        // size-size
        { "encoding", 
            [](BuildConfig& cfg, auto tokens) {
                cfg.Encoding.Key.ContentKey = CKey { tokens[1] };
                if (tokens.size() == 3)
                    cfg.Encoding.Key.EncodingKey = EKey { tokens[2] };
            }
        }, { "encoding-size",
            [](BuildConfig& cfg, auto tokens) {
                std::from_chars(tokens[1].data(), tokens[1].data() + tokens[1].size(), cfg.Encoding.Size[0]);
                if (tokens.size() == 3)
                    std::from_chars(tokens[2].data(), tokens[2].data() + tokens[2].size(), cfg.Encoding.Size[1]);
            }
        }, { "build-name",
            [](BuildConfig& cfg, auto tokens) {
                cfg.BuildName = tokens[1];
            }
        }
    };

    BuildConfig::BuildConfig(io::IReadableStream& fileStream) {
        fileStream.SeekRead(0);
        std::string fileContents;
        fileStream.ReadString(fileContents, fileStream.GetLength());

        std::vector<std::string_view> lines = ext::Tokenize(std::string_view { fileContents }, '\n');

        for (std::string_view line : lines) {
            if (line.empty() || line[0] == '#')
                continue;

            static const char Separators[] = { ' ', '=' };
            std::vector<std::string_view> tokens = ext::Tokenize(line, std::span{ Separators }, true);

            for (auto&& handler : Handlers) {
                if (tokens[0] != handler.Token)
                    continue;

                handler.Handler(*this, std::move(tokens));
                break;
            }
        }
    }
}

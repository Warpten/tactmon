#include "libtactmon/detail/Tokenizer.hpp"
#include "libtactmon/tact/config/BuildConfig.hpp"
#include "libtactmon/io/IReadableStream.hpp"

#include <charconv>
#include <string_view>
#include <vector>

using namespace std::string_view_literals;

namespace libtactmon::tact::config {
    struct ConfigHandler {
        std::string_view Token;

        using HandlerType = bool(*)(BuildConfig&, std::vector<std::string_view>);
        HandlerType Handler;
    };

    // Not all properties are modeled here.
    static const ConfigHandler Handlers[] = {
        { "root",
            [](BuildConfig& cfg, std::vector<std::string_view> tokens) {
                if (tokens.size() != 2)
                    return false;

                if (!CKey::TryParse(tokens[1], cfg.Root))
                    return false;

                return true;
            }
        }, { "install",
            [](BuildConfig& cfg, std::vector<std::string_view> tokens) {
                if (tokens.size() != 3 && tokens.size() != 2)
                    return false;

                if (!CKey::TryParse(tokens[1], cfg.Install.Key.ContentKey))
                    return false;

                if (!EKey::TryParse(tokens[2], cfg.Install.Key.EncodingKey))
                    return false;

                return true;
            }
        }, { "install-size",
            [](BuildConfig& cfg, std::vector<std::string_view> tokens) {
                if (tokens.size() != 3 && tokens.size() != 2)
                    return false;

                {
                    auto [ptr, ec] = std::from_chars(tokens[1].data(), tokens[1].data() + tokens[1].size(), cfg.Install.Size[0]);
                    if (ec != std::errc{ })
                        return false;
                }

                if (tokens.size() == 3) {
                    auto [ptr, ec] = std::from_chars(tokens[2].data(), tokens[2].data() + tokens[2].size(), cfg.Install.Size[1]);
                    if (ec != std::errc{ })
                        return false;
                }

                return true;
            }
        }, { "encoding", 
            [](BuildConfig& cfg, std::vector<std::string_view> tokens) {
                if (tokens.size() != 3 && tokens.size() != 2)
                    return false;

                if (!CKey::TryParse(tokens[1], cfg.Encoding.Key.ContentKey))
                    return false;

                if (!EKey::TryParse(tokens[2], cfg.Encoding.Key.EncodingKey))
                    return false;

                return true;
            }
        }, { "encoding-size",
            [](BuildConfig& cfg, std::vector<std::string_view> tokens) {
                if (tokens.size() != 3 && tokens.size() != 2)
                    return false;

                {
                    auto [ptr, ec] = std::from_chars(tokens[1].data(), tokens[1].data() + tokens[1].size(), cfg.Encoding.Size[0]);
                    if (ec != std::errc{ })
                        return false;
                }

                if (tokens.size() == 3) {
                    auto [ptr, ec] = std::from_chars(tokens[2].data(), tokens[2].data() + tokens[2].size(), cfg.Encoding.Size[1]);
                    if (ec != std::errc{ })
                        return false;
                }

                return true;
            }
        }, { "build-name",
            [](BuildConfig& cfg, std::vector<std::string_view> tokens) {
                if (tokens.size() != 2)
                    return false;

                cfg.BuildName = tokens[1];
                return true;
            }
        }
    };
    
    /* static */ std::optional<BuildConfig> BuildConfig::Parse(io::IReadableStream& stream) {
        stream.SeekRead(0);
        
        std::string_view contents { stream.Data<char>().data(), stream.GetLength() };
        std::vector<std::string_view> lines = detail::Tokenize(contents, '\n');
        if (lines.empty())
            return std::nullopt;

        BuildConfig config { };

        for (std::string_view line : lines) {
            if (line.empty() || line[0] == '#')
                continue;

            static const char Separators[] = { ' ', '=' };
            std::vector<std::string_view> tokens = detail::Tokenize(line, std::span { Separators }, true);

            for (auto&& handler : Handlers) {
                if (tokens[0] != handler.Token)
                    continue;

                if (!handler.Handler(config, std::move(tokens)))
                    return std::nullopt;
                break;
            }
        }

        return config;
    }
}

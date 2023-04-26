#include "libtactmon/detail/Tokenizer.hpp"
#include "libtactmon/tact/config/BuildConfig.hpp"
#include "libtactmon/io/IReadableStream.hpp"
#include "libtactmon/Errors.hpp"

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
                    return Error::BuildConfig_InvalidRoot;

                if (!CKey::TryParse(tokens[1], cfg.Root))
                    return Error::BuildConfig_InvalidRoot;

                return Error::OK;
            }
        }, { "install",
            [](BuildConfig& cfg, std::vector<std::string_view> tokens) {
                if (tokens.size() != 3 && tokens.size() != 2)
                    return Error::BuildConfig_InvalidInstall;

                if (!CKey::TryParse(tokens[1], cfg.Install.Key.ContentKey))
                    return Error::BuildConfig_InvalidInstall;

                if (!EKey::TryParse(tokens[2], cfg.Install.Key.EncodingKey))
                    return Error::BuildConfig_InvalidInstall;

                return Error::OK;
            }
        }, { "install-size",
            [](BuildConfig& cfg, std::vector<std::string_view> tokens) {
                if (tokens.size() != 3 && tokens.size() != 2)
                    return Error::BuildConfig_InvalidInstallSize;

                {
                    auto [ptr, ec] = std::from_chars(tokens[1].data(), tokens[1].data() + tokens[1].size(), cfg.Install.Size[0]);
                    if (ec != std::errc{ })
                        return Error::BuildConfig_InvalidInstallSize;
                }

                if (tokens.size() == 3) {
                    auto [ptr, ec] = std::from_chars(tokens[2].data(), tokens[2].data() + tokens[2].size(), cfg.Install.Size[1]);
                    if (ec != std::errc{ })
                        return Error::BuildConfig_InvalidInstallSize;
                }

                return Error::OK;
            }
        }, { "encoding", 
            [](BuildConfig& cfg, std::vector<std::string_view> tokens) {
                if (tokens.size() != 3 && tokens.size() != 2)
                    return Error::BuildConfig_InvalidEncoding;

                if (!CKey::TryParse(tokens[1], cfg.Encoding.Key.ContentKey))
                    return Error::BuildConfig_InvalidEncoding;

                if (!EKey::TryParse(tokens[2], cfg.Encoding.Key.EncodingKey))
                    return Error::BuildConfig_InvalidEncoding;

                return Error::OK;
            }
        }, { "encoding-size",
            [](BuildConfig& cfg, std::vector<std::string_view> tokens) {
                if (tokens.size() != 3 && tokens.size() != 2)
                    return Error::BuildConfig_InvalidEncodingSize;

                {
                    auto [ptr, ec] = std::from_chars(tokens[1].data(), tokens[1].data() + tokens[1].size(), cfg.Encoding.Size[0]);
                    if (ec != std::errc{ })
                        return Error::BuildConfig_InvalidEncodingSize;
                }

                if (tokens.size() == 3) {
                    auto [ptr, ec] = std::from_chars(tokens[2].data(), tokens[2].data() + tokens[2].size(), cfg.Encoding.Size[1]);
                    if (ec != std::errc{ })
                        return Error::BuildConfig_InvalidEncodingSize;
                }

                return Error::OK;
            }
        }, { "build-name",
            [](BuildConfig& cfg, std::vector<std::string_view> tokens) {
                if (tokens.size() != 2)
                    return Error::BuildConfig_InvalidBuildName;

                cfg.BuildName = tokens[1];
                return Error::OK;
            }
        }
    };
    
    /* static */ Result<BuildConfig> BuildConfig::Parse(io::IReadableStream& stream) {
        stream.SeekRead(0);
        
        std::string_view contents { stream.Data<char>().data(), stream.GetLength() };
        std::vector<std::string_view> lines = libtactmon::detail::Tokenize(contents, '\n', false);
        if (lines.empty())
            return Result<BuildConfig> { Error::MalformedBuildConfiguration };

        BuildConfig config { };

        for (std::string_view line : lines) {
            if (line[0] == '#')
                continue;

            static const char Separators[] = { ' ', '=' };
            std::vector<std::string_view> tokens = libtactmon::detail::Tokenize(line, std::span { Separators }, true);

            for (auto&& handler : Handlers) {
                if (tokens[0] != handler.Token)
                    continue;

                Error error = handler.Handler(config, std::move(tokens));
                if (error != Error::OK)
                    return Result<BuildConfig> { error };
                break;
            }
        }

        return Result<BuildConfig> { std::move(config) };
    }

    BuildConfig::BuildConfig(BuildConfig&& other) noexcept
        : Root(std::move(other.Root)), Install(std::move(other.Install)), Encoding(std::move(other.Encoding)), BuildName(std::move(other.BuildName))
    {

    }

    BuildConfig& BuildConfig::operator = (BuildConfig&& other) noexcept {
        Root = std::move(other.Root);
        Install = std::move(other.Install);
        Encoding = std::move(other.Encoding);
        BuildName = std::move(other.BuildName);

        return *this;
    }
}

#include "libtactmon/detail/Tokenizer.hpp"
#include "libtactmon/tact/config/BuildConfig.hpp"
#include "libtactmon/io/IReadableStream.hpp"
#include "libtactmon/Errors.hpp"

#include <charconv>
#include <string_view>
#include <vector>

using namespace std::string_view_literals;

namespace libtactmon::tact::config {
    using namespace errors;

    struct ConfigHandler {
        std::string_view Token;

        using HandlerType = Error(*)(BuildConfig&, std::vector<std::string_view>);
        HandlerType Handler;
    };

    // Not all properties are modeled here.
    static const ConfigHandler Handlers[] = {
        { "root",
            [](BuildConfig& cfg, std::vector<std::string_view> tokens) {
                if (tokens.size() != 2)
                    return errors::cfg::InvalidPropertySpecification("root", tokens);

                if (!CKey::TryParse(tokens[1], cfg.Root))
                    return errors::cfg::InvalidContentKey(tokens[1]);

                return errors::Success;
            }
        }, { "install",
            [](BuildConfig& cfg, std::vector<std::string_view> tokens) {
                if (tokens.size() != 3 && tokens.size() != 2)
                    return errors::cfg::InvalidPropertySpecification("install", tokens);

                if (!CKey::TryParse(tokens[1], cfg.Install.Key.ContentKey))
                    return errors::cfg::InvalidContentKey(tokens[1]);

                if (!EKey::TryParse(tokens[2], cfg.Install.Key.EncodingKey))
                    return errors::cfg::InvalidEncodingKey(tokens[2]);

                return errors::Success;
            }
        }, { "install-size",
            [](BuildConfig& cfg, std::vector<std::string_view> tokens) {
                if (tokens.size() != 3 && tokens.size() != 2)
                    return errors::cfg::InvalidPropertySpecification("install-size", tokens);

                {
                    auto [ptr, ec] = std::from_chars(tokens[1].data(), tokens[1].data() + tokens[1].size(), cfg.Install.Size[0]);
                    if (ec != std::errc{ })
                        return errors::cfg::InvalidPropertySpecification("install-size", tokens);
                }

                if (tokens.size() == 3) {
                    auto [ptr, ec] = std::from_chars(tokens[2].data(), tokens[2].data() + tokens[2].size(), cfg.Install.Size[1]);
                    if (ec != std::errc{ })
                        return errors::cfg::InvalidPropertySpecification("install-size", tokens);
                }

                return errors::Success;
            }
        }, { "encoding", 
            [](BuildConfig& cfg, std::vector<std::string_view> tokens) {
                if (tokens.size() != 3 && tokens.size() != 2)
                    return errors::cfg::InvalidPropertySpecification("encoding", tokens);

                if (!CKey::TryParse(tokens[1], cfg.Encoding.Key.ContentKey))
                    return errors::cfg::InvalidContentKey(tokens[1]);

                if (!EKey::TryParse(tokens[2], cfg.Encoding.Key.EncodingKey))
                    return errors::cfg::InvalidEncodingKey(tokens[2]);

                return errors::Success;
            }
        }, { "encoding-size",
            [](BuildConfig& cfg, std::vector<std::string_view> tokens) {
                if (tokens.size() != 3 && tokens.size() != 2)
                    return errors::cfg::InvalidPropertySpecification("encoding-size", tokens);

                {
                    auto [ptr, ec] = std::from_chars(tokens[1].data(), tokens[1].data() + tokens[1].size(), cfg.Encoding.Size[0]);
                    if (ec != std::errc{ })
                        return errors::cfg::InvalidPropertySpecification("encoding-size", tokens);
                }

                if (tokens.size() == 3) {
                    auto [ptr, ec] = std::from_chars(tokens[2].data(), tokens[2].data() + tokens[2].size(), cfg.Encoding.Size[1]);
                    if (ec != std::errc{ })
                        return errors::cfg::InvalidPropertySpecification("encoding-size", tokens);
                }

                return errors::Success;
            }
        }, { "build-name",
            [](BuildConfig& cfg, std::vector<std::string_view> tokens) {
                if (tokens.size() != 2)
                    return errors::cfg::InvalidPropertySpecification("build-name", tokens);

                cfg.BuildName = tokens[1];
                return errors::Success;
            }
        }
    };
    
    /* static */ Result<BuildConfig> BuildConfig::Parse(io::IReadableStream& stream) {
        stream.SeekRead(0);
        
        std::string_view contents { stream.Data<char>().data(), stream.GetLength() };
        std::vector<std::string_view> lines = libtactmon::detail::CharacterTokenizer<'\n'> { contents, false }.Accumulate();
        if (lines.empty())
            return Result<BuildConfig> { errors::cfg::MalformedFile("") };

        BuildConfig config { };

        for (std::string_view line : lines) {
            if (line[0] == '#')
                continue;

            std::vector<std::string_view> tokens = libtactmon::detail::ConfigurationTokenizer { line, true }.Accumulate();
            for (auto&& handler : Handlers) {
                if (tokens[0] != handler.Token)
                    continue;

                Error error = handler.Handler(config, std::move(tokens));
                if (error != errors::Success)
                    return Result<BuildConfig> { std::move(error) };
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

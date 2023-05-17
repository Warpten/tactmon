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
        using MatcherType = bool(*)(std::vector<std::string_view>);
        MatcherType Matcher;

        using HandlerType = Error(*)(BuildConfig&, std::vector<std::string_view>);
        HandlerType Handler;
    };

#define MAKE_MATCHER(TOKEN) [](std::vector<std::string_view> tokens) { return !tokens.empty() && tokens[0] == TOKEN; }

    // Not all properties are modeled here.
    static const ConfigHandler Handlers[] = {
        {
            MAKE_MATCHER("root"),
            [](BuildConfig& cfg, std::vector<std::string_view> tokens) {
                if (tokens.size() != 2)
                    return errors::cfg::InvalidPropertySpecification("root", tokens);

                if (!CKey::TryParse(tokens[1], cfg.Root))
                    return errors::cfg::InvalidContentKey(tokens[1]);

                return errors::Success;
            }
        }, {
            MAKE_MATCHER("install"),
            [](BuildConfig& cfg, std::vector<std::string_view> tokens) {
                if (tokens.size() != 3 && tokens.size() != 2)
                    return errors::cfg::InvalidPropertySpecification("install", tokens);

                if (!CKey::TryParse(tokens[1], cfg.Install.Key.ContentKey))
                    return errors::cfg::InvalidContentKey(tokens[1]);

                if (!EKey::TryParse(tokens[2], cfg.Install.Key.EncodingKey))
                    return errors::cfg::InvalidEncodingKey(tokens[2]);

                return errors::Success;
            }
        }, {
            MAKE_MATCHER("install-size"),
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
        }, {
            MAKE_MATCHER("encoding"), 
            [](BuildConfig& cfg, std::vector<std::string_view> tokens) {
                if (tokens.size() != 3 && tokens.size() != 2)
                    return errors::cfg::InvalidPropertySpecification("encoding", tokens);

                if (!CKey::TryParse(tokens[1], cfg.Encoding.Key.ContentKey))
                    return errors::cfg::InvalidContentKey(tokens[1]);

                if (!EKey::TryParse(tokens[2], cfg.Encoding.Key.EncodingKey))
                    return errors::cfg::InvalidEncodingKey(tokens[2]);

                return errors::Success;
            }
        }, {
            MAKE_MATCHER("encoding-size"),
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
        }, {
            MAKE_MATCHER("build-name"),
            [](BuildConfig& cfg, std::vector<std::string_view> tokens) {
                if (tokens.size() != 2)
                    return errors::cfg::InvalidPropertySpecification("build-name", tokens);

                cfg.BuildName = tokens[1];
                return errors::Success;
            }
        }, {
            MAKE_MATCHER("build-uid"),
            [](BuildConfig& cfg, std::vector<std::string_view> tokens) {
                if (tokens.size() != 2)
                    return errors::cfg::InvalidPropertySpecification("build-uid", tokens);

                cfg.BuildUID = tokens[1];
                return errors::Success;
            }
        }, {
            MAKE_MATCHER("build-product"),
            [](BuildConfig& cfg, std::vector<std::string_view> tokens) {
                if (tokens.size() != 2)
                    return errors::cfg::InvalidPropertySpecification("build-product", tokens);

                cfg.BuildProduct = tokens[1];
                return errors::Success;
            }
        }, {
            MAKE_MATCHER("build-playbuild-installer"),
            [](BuildConfig& cfg, std::vector<std::string_view> tokens) {
                if (tokens.size() != 2)
                    return errors::cfg::InvalidPropertySpecification("build-playbuild-installer", tokens);

                cfg.BuildPlaybuildInstaller = tokens[1];
                return errors::Success;
            }
        }, {
            [](std::vector<std::string_view> tokens) {
                return !tokens.empty() && tokens[0].starts_with("vfs-");
            },
            [](BuildConfig& cfg, std::vector<std::string_view> tokens) {
                std::string_view propertySpecifier = tokens[0].substr(4); // vfs-
                if (propertySpecifier == "root") {
                    if (tokens.size() != 2)
                        return errors::cfg::InvalidPropertySpecification(tokens[0], tokens);

                    cfg.VFS.Root.Name = propertySpecifier;
                    return errors::Success;
                }

                if (propertySpecifier == "root-size") {
                    if (tokens.size() != 3)
                        return errors::cfg::InvalidPropertySpecification(tokens[0], tokens);

                    for (std::size_t i = 0; i < 2; ++i) {
                        auto [ptr, ec] = std::from_chars(tokens[i + 1].data(), tokens[i + 1].data() + tokens[i + 1].size(), cfg.VFS.Root.Size[i]);
                        if (ec != std::errc{ })
                            return errors::cfg::InvalidPropertySpecification(tokens[0], tokens);
                    }

                    return errors::Success;
                }

                std::size_t index = 0;
                auto [ptr, ec] = std::from_chars(propertySpecifier.data(), propertySpecifier.data() + propertySpecifier.size(), index);
                if (ec != std::errc{ })
                    return errors::cfg::InvalidPropertySpecification(tokens[0], tokens);

                if (cfg.VFS.Entries.size() < index)
                    cfg.VFS.Entries.resize(index);

                if (ptr != propertySpecifier.data() + propertySpecifier.size()) {
                    if (tokens.size() != 3)
                        return errors::cfg::InvalidPropertySpecification(tokens[0], tokens);

                    for (std::size_t i = 0; i < 2; ++i) {
                        auto [ptr, ec] = std::from_chars(tokens[i + 1].data(), tokens[i + 1].data() + tokens[i + 1].size(), cfg.VFS.Entries[index].Size[i]);
                        if (ec != std::errc{ })
                            return errors::cfg::InvalidPropertySpecification(tokens[0], tokens);
                    }

                    return errors::Success;
                }

                if (tokens.size() != 2)
                    return errors::cfg::InvalidPropertySpecification(tokens[0], tokens);

                cfg.VFS.Entries[index].Name = tokens[1];
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
                if (!handler.Matcher(tokens))
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

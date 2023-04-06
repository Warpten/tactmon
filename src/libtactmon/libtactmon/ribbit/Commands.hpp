#pragma once

#include "libtactmon/ribbit/Enums.hpp"
#include "libtactmon/ribbit/types/BGDL.hpp"
#include "libtactmon/ribbit/types/CDNs.hpp"
#include "libtactmon/ribbit/types/Summary.hpp"
#include "libtactmon/ribbit/types/Versions.hpp"

#include <cstdint>
#include <functional>
#include <optional>
#include <sstream>
#include <string_view>

#include <boost/algorithm/string.hpp>
#include <boost/algorithm/string_regex.hpp>
#include <boost/asio/connect.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/buffers_iterator.hpp>
#include <boost/asio/read.hpp>
#include <boost/asio/streambuf.hpp>
#include <boost/asio/write.hpp>
#include <boost/core/ignore_unused.hpp>
#include <boost/tokenizer.hpp>

#include <fmt/format.h>

#include <spdlog/spdlog.h>

#include "libtactmon/detail/ctre.hpp"

namespace libtactmon::ribbit {
    namespace detail {
        template <Command> struct CommandTraits;

        template <> struct CommandTraits<Command::Summary> {
            constexpr static const char Format[] = "{}/summary";

            using Args = std::tuple<>;
            using ValueType = types::Summary;

            static std::optional<types::Summary> Parse(std::string_view input);
        };

        template <> struct CommandTraits<Command::ProductVersions> {
            constexpr static const char Format[] = "{}/products/{}/versions";

            using Args = std::tuple<std::string_view>;
            using ValueType = types::Versions;

            static std::optional<types::Versions> Parse(std::string_view input);
        };

        template <> struct CommandTraits<Command::ProductCDNs> {
            constexpr static const char Format[] = "{}/products/{}/cdns";

            using Args = std::tuple<std::string_view>;
            using ValueType = types::CDNs;

            static std::optional<types::CDNs> Parse(std::string_view input);
        };

        template <> struct CommandTraits<Command::ProductBGDL> {
            constexpr static const char Format[] = "{}/products/{}/cdns";

            using ValueType = types::BGDL;
            using Args = std::tuple<std::string_view>;

            static std::optional<types::BGDL> Parse(std::string_view input);
        };

        template <Version> struct VersionTraits;

        template <> struct VersionTraits<Version::V1> {
            constexpr static const std::string_view Value = "v1";

            template <typename C>
            static auto Parse(std::string_view payload, spdlog::logger* logger)
                -> std::optional<typename C::ValueType>
            {
                std::vector<std::string_view> messageParts = ParseCore(payload, logger);
                for (std::string_view elem : messageParts) {
                    auto elemParse = C::Parse(elem);
                    if (elemParse.has_value())
                        return elemParse;
                }

                return std::nullopt;
            }

        private:
            static std::vector<std::string_view> ParseCore(std::string_view input, spdlog::logger* logger);
        };

        template <> struct VersionTraits<Version::V2> {
            constexpr static const std::string_view Value = "v2";

            template <typename C>
            static auto Parse(std::string_view payload, spdlog::logger* logger)
                -> std::optional<typename C::ValueType>
            {
                std::vector<std::string_view> messageParts = ParseCore(payload, logger);
                for (std::string_view elem : messageParts) {
                    auto elemParse = C::Parse(elem);
                    if (elemParse.has_value())
                        return elemParse;
                }

                return std::nullopt;
            }

        private:
            static std::vector<std::string_view> ParseCore(std::string_view input, spdlog::logger* logger);
        };

        template <Command C, Version V, typename Args> class command_executor_impl;
        template <Command C, Version V, typename... Args>
        class command_executor_impl<C, V, std::tuple<Args...>> {
            using CommandTraits = detail::CommandTraits<C>;
            using VersionTraits = detail::VersionTraits<V>;

        public:
            static auto Execute(boost::asio::any_io_executor executor, Region region, Args&&... args) {
                return Execute(executor, nullptr, region, std::forward<Args&&>(args)...);
            }

            static auto Execute(boost::asio::any_io_executor executor, spdlog::logger* logger, Region region, Args&&... args)
                -> std::optional<typename CommandTraits::ValueType>
            {
                namespace asio = boost::asio;
                namespace ip = asio::ip;
                using tcp = ip::tcp;

                ip::tcp::socket socket{ executor };

                boost::system::error_code ec;

                auto command = fmt::format(CommandTraits::Format, VersionTraits::Value, std::forward<Args>(args)...) + "\r\n";

                std::string host = fmt::format("{}.version.battle.net", region);

                if (logger != nullptr)
                    logger->info("Loading {}:{}/{}.", host, 1119, command);

                tcp::resolver r{ executor };
                asio::connect(socket, r.resolve(host, "1119"), ec);

                if (ec) {
                    if (logger != nullptr)
                        logger->error("An error occured: {}.", ec.message());

                    return std::nullopt;
                }

                asio::write(socket, asio::buffer(command), ec);
                if (ec) {
                    if (logger != nullptr)
                        logger->error("An error occured: {}.", ec.message());

                    return std::nullopt;
                }

                { // Read Ribbit response
                    boost::asio::streambuf buf;
                    std::size_t bytesTransferred = asio::read(socket, buf, ec);
                    boost::ignore_unused(bytesTransferred);
                    if (ec && ec != boost::asio::error::eof) {
                        if (logger != nullptr)
                            logger->error("An error occured: {}.", ec.message());

                        return std::nullopt;
                    }

                    auto bufs = buf.data();
                    std::string response { asio::buffers_begin(bufs), asio::buffers_begin(bufs) + buf.size() };

                    return VersionTraits::template Parse<CommandTraits>(response, logger);
                }
            }
        };
    }

    template <Command C, Version V, typename Args = typename detail::CommandTraits<C>::Args>
    struct CommandExecutor final : detail::command_executor_impl<C, V, Args> { };

    template <Version V = Version::V1>
    using CDNs = CommandExecutor<Command::ProductCDNs, V>;

    template <Version V = Version::V1>
    using Versions = CommandExecutor<Command::ProductVersions, V>;

    template <Version V = Version::V1>
    using Summary = CommandExecutor<Command::Summary, V>;
}

#pragma once

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
    enum class Region : uint8_t {
        EU = 1,
        US = 2,
        KR = 3,
        CN = 4,
        TW = 5
    };

    enum class Version : uint8_t {
        V1 = 1,
        V2 = 2
    };

    enum class Command : uint8_t {
        Summary = 0,
        ProductVersions = 1,
        ProductCDNs = 2,
        ProductBGDL = 3,
        Certificate = 4,
        OCSP = 5,
    };

    namespace detail {
        template <Region> struct RegionTraits;
        template <> struct RegionTraits<Region::EU> { constexpr static const char Host[] = "eu.version.battle.net"; };
        template <> struct RegionTraits<Region::US> { constexpr static const char Host[] = "us.version.battle.net"; };
        template <> struct RegionTraits<Region::KR> { constexpr static const char Host[] = "kr.version.battle.net"; };
        template <> struct RegionTraits<Region::CN> { constexpr static const char Host[] = "cn.version.battle.net"; };
        template <> struct RegionTraits<Region::TW> { constexpr static const char Host[] = "tw.version.battle.net"; };

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

        // template <> struct CommandTraits<Command::Certificate> {
        // 	constexpr static const std::string_view Format = "{}/certs/{}";
        // 
        // 	using Args = std::tuple<std::string_view>;
        // };
        // template <> struct CommandTraits<Command::OCSP> {
        // 	constexpr static const std::string_view Format = "{}/ocsp/{}";
        // 
        // 	using Args = std::tuple<std::string_view>;
        // };

        template <Version> struct VersionTraits;

        template <> struct VersionTraits<Version::V1> {
            constexpr static const std::string_view Value = "v1";

            template <typename C>
            static auto Parse(std::string_view payload, std::shared_ptr<spdlog::logger> logger)
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
            static std::vector<std::string_view> ParseCore(std::string_view input, std::shared_ptr<spdlog::logger> logger);
        };

        template <> struct VersionTraits<Version::V2> {
            constexpr static const std::string_view Value = "v2";

            template <typename C>
            static auto Parse(std::string_view payload, std::shared_ptr<spdlog::logger> logger)
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
            static std::vector<std::string_view> ParseCore(std::string_view input, std::shared_ptr<spdlog::logger> logger);
        };
    }

    template <Command C, Region R, Version V>
    struct CommandExecutor {
        using CommandTraits = detail::CommandTraits<C>;
        using RegionTraits  = detail::RegionTraits<R>;
        using VersionTraits = detail::VersionTraits<V>;

        explicit CommandExecutor(boost::asio::io_context& ctx) : _socket(ctx), _ctx(ctx) { }

        template <typename... Args>
        requires std::is_same_v<std::tuple<Args...>, typename CommandTraits::Args>
        auto operator() (std::shared_ptr<spdlog::logger> logger, Args&&... args)
            -> std::optional<typename CommandTraits::ValueType>
        {
            namespace ip = boost::asio::ip;
            using tcp = ip::tcp;

            boost::system::error_code ec;

            auto command = fmt::format(CommandTraits::Format, VersionTraits::Value, std::forward<Args>(args)...) + "\r\n";

            if (logger != nullptr) {
                logger->info("Loading {}:{}/{}.", RegionTraits::Host, 1119, command);
                logger->trace("Connecting to {}:{}/{}.", RegionTraits::Host, 1119, command);
            }

            tcp::resolver r { _ctx };
            boost::asio::connect(_socket, r.resolve(RegionTraits::Host, "1119"), ec);

            if (ec) {
                if (logger != nullptr)
                    logger->error("An error occured: {}.", ec.message());

                return std::nullopt;
            }

            { // Write Ribbit request
                auto buf = boost::asio::buffer(command);
                boost::asio::write(_socket, buf, ec);
                if (ec) {
                    if (logger != nullptr)
                        logger->error("An error occured: {}.", ec.message());

                    return std::nullopt;
                }
            }

            { // Read Ribbit response
                boost::asio::streambuf buf;
                size_t bytesTransferred = boost::asio::read(_socket, buf, ec);
                boost::ignore_unused(bytesTransferred);
                if (ec && ec != boost::asio::error::eof) {
                    if (logger != nullptr)
                        logger->error("An error occured: {}.", ec.message());

                    return std::nullopt;
                }

                auto bufs = buf.data();
                std::string response { boost::asio::buffers_begin(bufs), boost::asio::buffers_begin(bufs) + buf.size() };

                return VersionTraits::template Parse<CommandTraits>(response, logger);
            }
        }

    private:
        boost::asio::io_context& _ctx;
        boost::asio::ip::tcp::socket _socket;
    };

    template <Region R, Version V = Version::V1>
    using CDNs = CommandExecutor<Command::ProductCDNs, R, V>;

    template <Region R, Version V = Version::V1>
    using Versions = CommandExecutor<Command::ProductVersions, R, V>;

    template <Region R, Version V = Version::V1>
    using Summary = CommandExecutor<Command::Summary, R, V>;
}

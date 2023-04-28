#pragma once

#include "libtactmon/detail/ctre.hpp"
#include "libtactmon/detail/Export.hpp"
#include "libtactmon/ribbit/Enums.hpp"
#include "libtactmon/ribbit/types/BGDL.hpp"
#include "libtactmon/ribbit/types/CDNs.hpp"
#include "libtactmon/ribbit/types/Summary.hpp"
#include "libtactmon/ribbit/types/Versions.hpp"
#include "libtactmon/Errors.hpp"
#include "libtactmon/Result.hpp"

#include <cstdint>
#include <functional>
#include <optional>
#include <sstream>
#include <string_view>

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

namespace libtactmon::ribbit {
    namespace detail {
        template <Command> struct CommandTraits;

        template <> struct CommandTraits<Command::Summary> {
            constexpr static const char Format[] = "{}/summary";

            using Args = std::tuple<>;
            using ValueType = types::Summary;

            static Result<types::Summary> Parse(std::string_view command, std::string_view input);
        };

        template <> struct CommandTraits<Command::ProductVersions> {
            constexpr static const char Format[] = "{}/products/{}/versions";

            using Args = std::tuple<std::string_view>;
            using ValueType = types::Versions;

            static Result<types::Versions> Parse(std::string_view command, std::string_view input);
        };

        template <> struct CommandTraits<Command::ProductCDNs> {
            constexpr static const char Format[] = "{}/products/{}/cdns";

            using Args = std::tuple<std::string_view>;
            using ValueType = types::CDNs;

            static Result<types::CDNs> Parse(std::string_view command, std::string_view input);
        };

        template <> struct CommandTraits<Command::ProductBGDL> {
            constexpr static const char Format[] = "{}/products/{}/cdns";

            using ValueType = types::BGDL;
            using Args = std::tuple<std::string_view>;

            static Result<types::BGDL> Parse(std::string_view command, std::string_view input);
        };

        template <Version> struct VersionTraits;

        template <> struct VersionTraits<Version::V1> {
            constexpr static const std::string_view Value = "v1";

            template <typename C>
            static auto Parse(std::string_view command, std::string_view payload)
                -> Result<typename C::ValueType>
            {
                return ParseCore(command, payload).transform([&](std::vector<std::string_view> tokens) {
                    for (std::string_view token : tokens) {
                        auto elemParse = C::Parse(command, token);
                        if (elemParse.has_value())
                            return elemParse;
                    }

                    return Result<typename C::ValueType> { errors::ribbit::Unparsable(command) };
                });
            }

        private:
            static Result<std::vector<std::string_view>> ParseCore(std::string_view command, std::string_view input);
        };

        template <> struct VersionTraits<Version::V2> {
            constexpr static const std::string_view Value = "v2";

            template <typename C>
            static auto Parse(std::string_view command, std::string_view payload)
                -> Result<typename C::ValueType>
            {
                std::vector<std::string_view> messageParts = ParseCore(payload);
                for (std::string_view elem : messageParts) {
                    auto elemParse = C::Parse(command, elem);
                    if (elemParse.has_value())
                        return elemParse;
                }

                return Result<typename C::ValueType> { errors::ribbit::Unparsable(command) };
            }

        private:
            static std::vector<std::string_view> ParseCore(std::string_view input);
        };

        template <Command C, Version V, typename Args> class command_executor_impl;
        template <Command C, Version V, typename... Args>
        class command_executor_impl<C, V, std::tuple<Args...>> {
            using CommandTraits = detail::CommandTraits<C>;
            using VersionTraits = detail::VersionTraits<V>;

        public:
            static auto Execute(boost::asio::any_io_executor const& executor, Region region, Args... args)
                -> Result<typename CommandTraits::ValueType>
            {
                using result_type = Result<typename CommandTraits::ValueType>;

                namespace error = libtactmon::errors;

                namespace asio = boost::asio;
                namespace ip = asio::ip;
                using tcp = ip::tcp;

                ip::tcp::socket socket{ executor };

                boost::system::error_code ec;

                auto command = fmt::format(CommandTraits::Format, VersionTraits::Value, std::forward<Args>(args)...) + "\r\n";

                std::string host = fmt::format("{}.version.battle.net", region);

                tcp::resolver r { executor };
                asio::connect(socket, r.resolve(host, "1119"), ec);

                if (ec.failed()) return result_type { error::network::ConnectError(ec, host) };

                asio::write(socket, asio::buffer(command), ec);
                if (ec.failed()) return result_type { error::network::WriteError(ec, host) };

                { // Read Ribbit response
                    boost::asio::streambuf buf;
                    std::size_t bytesTransferred = asio::read(socket, buf, ec);
                    boost::ignore_unused(bytesTransferred);
                    if (ec && ec != boost::asio::error::eof)
                        return result_type{ error::network::ReadError(ec, host) };

                    auto bufs = buf.data();
                    std::string response { asio::buffers_begin(bufs), asio::buffers_begin(bufs) + buf.size() };

                    return VersionTraits::template Parse<CommandTraits>(command, response);
                }
            }
        };
    }

    template <Command C, Version V, typename Args = typename detail::CommandTraits<C>::Args>
    struct LIBTACTMON_API CommandExecutor final : detail::command_executor_impl<C, V, Args> { };

    template <Version V = Version::V1>
    using CDNs = CommandExecutor<Command::ProductCDNs, V>;

    template <Version V = Version::V1>
    using Versions = CommandExecutor<Command::ProductVersions, V>;

    template <Version V = Version::V1>
    using Summary = CommandExecutor<Command::Summary, V>;
}

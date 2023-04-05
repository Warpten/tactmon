#include "libtactmon/ribbit/Commands.hpp"
#include "libtactmon/ribbit/types/BGDL.hpp"
#include "libtactmon/ribbit/types/CDNs.hpp"
#include "libtactmon/ribbit/types/Summary.hpp"
#include "libtactmon/ribbit/types/Versions.hpp"

#include <boost/beast/http.hpp>
#include <boost/tokenizer.hpp>

namespace libtactmon::ribbit::detail {
    using char_sep = boost::char_separator<char>;
    using tokenizer = boost::tokenizer<char_sep>;

    /* static */ std::vector<std::string_view> VersionTraits<Version::V1>::ParseCore(std::string_view input, spdlog::logger* logger) {
        // We pretend this is an HTTP response by shoving a "HTTP/1.1 200 OK\r\n" at the front of the response
        std::string httpResponse = "HTTP/1.1 200 OK\r\n";
        httpResponse += input;

        // And then we just parse it through Boost.Beast
        boost::beast::http::response_parser<boost::beast::http::string_body> parser;

        boost::beast::error_code ec;
        parser.put(boost::asio::buffer(httpResponse), ec);
        if (ec.failed()) {
            if (logger != nullptr)
                logger->error("An error occured: {}.", ec.message());

            return {};
        }

        auto mimeBoundary = [&]() -> std::optional<std::string_view> {
            auto contentType = parser.get()[boost::beast::http::field::content_type];

            auto r1 = ctre::match<R"reg(^multipart/([^;]+);\s*boundary="(?<boundary>[^"]+)"$)reg">(contentType.begin(), contentType.end());
            auto r2 = ctre::match<R"reg(^multipart/([^;]+);\s*boundary=(?<boundary>[^"]+)$)reg">(contentType.begin(), contentType.end());

            if (r1) return r1.get<"boundary">();
            if (r2) return r2.get<"boundary">();
            return {};
        }();

        if (mimeBoundary == std::nullopt) {
            if (logger != nullptr)
                logger->error("An error occured: Malformed multipart response; message boundary not found.");

            return {};
        }

        // Found a MIME boundary, split the body
        std::vector<std::string_view> messageParts;
        boost::split_regex(messageParts, input, boost::regex{ fmt::format("--{}\r\n", *mimeBoundary) });
        return messageParts;
    }

    /* static */ std::vector<std::string_view> VersionTraits<Version::V2>::ParseCore(std::string_view input, spdlog::logger* logger) {
        // Split on the header delimiter.
        std::vector<std::string_view> messageParts;
        boost::split_regex(messageParts, input, boost::regex { "\r\n\r\n" });
        return messageParts;
    }

    /* static */ std::optional<types::BGDL> CommandTraits<Command::ProductBGDL>::Parse(std::string_view input) {
        boost::char_separator<char> sep{ "\r\n" };
        std::string strInput{ input };
        tokenizer tok{ strInput, sep };

        types::BGDL bgdl;

        for (std::string const& line : tok) {
            auto value = types::bgdl::Record::Parse(line);
            if (value.has_value())
                bgdl.push_back(*value);
        }

        if (bgdl.empty())
            return std::nullopt;

        // Remove the first row - needed because there's nothing stopping it from parsing as a record
        bgdl.erase(bgdl.begin());

        return bgdl;
    }

    /* static */ std::optional<types::CDNs> CommandTraits<Command::ProductCDNs>::Parse(std::string_view input) {
        boost::char_separator<char> sep{ "\r\n" };
        std::string strInput{ input };
        tokenizer tok{ strInput, sep };

        types::CDNs cdns;

        for (std::string const& line : tok) {
            auto value = types::cdns::Record::Parse(line);
            if (value.has_value())
                cdns.push_back(*value);
        }

        if (cdns.empty())
            return std::nullopt;

        // Remove the first row - needed because there's nothing stopping it from parsing as a record
        cdns.erase(cdns.begin());

        return cdns;
    }

    /* static */ std::optional<types::Summary> CommandTraits<Command::Summary>::Parse(std::string_view input) {
        boost::char_separator<char> sep{ "\r\n" };
        std::string strInput{ input };
        tokenizer tok{ strInput, sep };

        types::Summary summary;

        for (std::string const& line : tok) {
            auto item = types::summary::Record::Parse(line);
            if (item.has_value())
                summary.push_back(*item);
        }

        // Do not remove the first row - it gets skipped because we expect a sequence number in one column and treat it as an integer
        if (summary.empty())
            return std::nullopt;

        return summary;
    }

    /* static */ std::optional<types::Versions> CommandTraits<Command::ProductVersions>::Parse(std::string_view input) {
        boost::char_separator<char> sep{ "\r\n" };
        std::string strInput{ input };
        tokenizer tok{ strInput, sep };

        types::Versions versions;

        for (std::string const& line : tok) {
            auto value = types::versions::Record::Parse(line);
            if (value.has_value())
                versions.Records.push_back(*value);
            else if (line.starts_with("## seqn = ")) {
                auto seqnStr = line.substr(10);
                auto [ptr, ec] = std::from_chars(seqnStr.data(), seqnStr.data() + seqnStr.size(), versions.SequenceID);
                if (ec != std::errc{ })
                    versions.SequenceID = 0;
            }
        }

        if (versions.Records.empty() || versions.SequenceID == 0)
            return std::nullopt;

        // Do not remove the first row - it gets skipped because we expect a build number in one column and treat it as an integer
        return versions;
    }
}

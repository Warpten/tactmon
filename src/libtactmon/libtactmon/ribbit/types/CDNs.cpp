#include "libtactmon/ribbit/types/CDNs.hpp"
#include "libtactmon/ribbit/Commands.hpp"

#include <boost/algorithm/string/classification.hpp>
#include <boost/algorithm/string/split.hpp>

namespace libtactmon::ribbit::types::cdns {
    std::optional<Record> Record::Parse(std::string_view input) {
        std::vector<std::string> tokens;
        boost::split(tokens, input, boost::is_any_of("|"), boost::token_compress_off);
        if (tokens.size() != 5)
            return std::nullopt;

        Record record{ };
        record.Name = tokens[0];
        record.Path = tokens[1];
        boost::split(record.Hosts, tokens[2], boost::is_any_of(" "), boost::token_compress_on);
        boost::split(record.Servers, tokens[3], boost::is_any_of(" "), boost::token_compress_on);
        record.ConfigPath = tokens[4];

        return record;
    }
}

#include "libtactmon/detail/Tokenizer.hpp"
#include "libtactmon/ribbit/types/BGDL.hpp"
#include "libtactmon/ribbit/Commands.hpp"

#include <charconv>

#include <boost/tokenizer.hpp>

namespace libtactmon::ribbit::types::bgdl {
    std::optional<Record> Record::Parse(std::string_view input) {
        std::vector<std::string_view> tokens = libtactmon::detail::Tokenize(input, '|');
        if (tokens.size() != 7)
            return std::nullopt;

        Record rec { };
        rec.Region = tokens[0];
        rec.BuildConfig = tokens[1];
        rec.CDNConfig = tokens[2];
        rec.KeyRing = tokens[3];

        rec.VersionsName = tokens[5];
        rec.ProductConfig = tokens[6];

        auto [ptr, ec] = std::from_chars(tokens[4].data(), tokens[4].data() + tokens[4].size(), rec.BuildID);
        if (ec != std::errc{ })
            return std::nullopt;
        
        return rec;
    }
}

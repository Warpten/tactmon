#include "net/ribbit/types/Summary.hpp"
#include "net/ribbit/Commands.hpp"
#include "ext/Tokenizer.hpp"

#include <charconv>

#include <boost/tokenizer.hpp>

namespace net::ribbit::types::summary {
    std::optional<Record> Record::Parse(std::string_view input) {
        std::vector<std::string_view> tokens = ext::Tokenize(input, '|');
        if (tokens.size() != 3 && tokens.size() != 2)
            return std::nullopt;

        Record rec { };
        rec.Product = tokens[0];

        rec.Flags = tokens.size() == 3 ? tokens[2] : "";

        auto [ptr, ec] = std::from_chars(tokens[1].data(), tokens[1].data() + tokens[1].size(), rec.SequenceID);
        if (ec != std::errc{ })
            return std::nullopt;

        return rec;
    }
}

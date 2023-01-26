#include "net/ribbit/types/Summary.hpp"
#include "net/ribbit/Commands.hpp"

#include <charconv>

#include <boost/tokenizer.hpp>

namespace net::ribbit::types::summary {
	std::optional<Record> Record::Parse(std::string_view input) {
		std::vector<std::string_view> tokens;

		while (!input.empty()) {
			size_t offset = input.find('|');
			if (offset == std::string_view::npos) {
				tokens.push_back(input);
				break;
			} else {
				tokens.push_back(input.substr(0, offset));
				input.remove_prefix(offset + 1);
			}
		}

		if (tokens.size() != 3)
			return std::nullopt;

		Record rec { };
		rec.Product = tokens[0];

		rec.Flags = tokens[2];

		auto [ptr, ec] = std::from_chars(tokens[1].data(), tokens[1].data() + tokens[1].size(), rec.SequenceID);
		if (ec != std::errc{ })
			return std::nullopt;

		return rec;
	}
}

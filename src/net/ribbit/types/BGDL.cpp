#include "net/ribbit/types/BGDL.hpp"
#include "net/ribbit/Commands.hpp"

#include <string_view>

#include <boost/tokenizer.hpp>

namespace net::ribbit::types::bgdl {
	std::optional<Record> Record::Parse(std::string_view input) {
		std::vector<std::string_view> tokens;

		while (!input.empty()) {
			size_t offset = input.find('|');
			if (offset == std::string_view::npos) {
				tokens.push_back(input);
				break;
			}
			else {
				tokens.push_back(input.substr(0, offset));
				input.remove_prefix(offset + 1);
			}
		}

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

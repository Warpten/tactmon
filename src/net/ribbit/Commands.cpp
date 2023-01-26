#include "net/ribbit/Commands.hpp"
#include "net/ribbit/types/BGDL.hpp"
#include "net/ribbit/types/CDNs.hpp"
#include "net/ribbit/types/Summary.hpp"
#include "net/ribbit/types/Versions.hpp"

#include <boost/tokenizer.hpp>

namespace net::ribbit {
	using char_sep = boost::char_separator<char>;
	using tokenizer = boost::tokenizer<char_sep>;

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
				versions.push_back(*value);
		}

		if (versions.empty())
			return std::nullopt;

		// Do not remove the first row - it gets skipped because we expect a build number in one column and treat it as an integer
		return versions;
	}
}

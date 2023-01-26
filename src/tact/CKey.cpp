#include "tact/CKey.hpp"

#include <boost/algorithm/hex.hpp>
#include <boost/algorithm/string.hpp>

namespace tact {
	CKey::CKey(std::string_view value) noexcept {
		boost::algorithm::unhex(value.begin(), value.end(), _value.begin());
	}

	std::string CKey::ToString() const {
		std::string result;
		result.reserve(_value.size() * 2);
		boost::algorithm::hex(_value.begin(), _value.end(), std::back_inserter(result));
		boost::algorithm::to_lower(result);
		return result;
	}
}

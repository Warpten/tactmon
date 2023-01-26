#pragma once

#include <optional>

#include <tact/EKey.hpp>

namespace io {
	struct IStream;
}

namespace tact {
	struct BLTE final {
		static std::optional<BLTE> Parse(io::IReadableStream& fstream, tact::EKey const& ekey);

	private:
		explicit BLTE(io::IReadableStream& fstream);

	public:

	private:
	};
}
#pragma once

#include <cstdint>
#include <list>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <unordered_map>

namespace io {
	struct IReadableStream;
}

namespace tact::data {
	struct Install final {
		static std::optional<Install> Parse(io::IReadableStream& stream);

		struct Tag final {
			Tag(Install const& install, io::IReadableStream& stream, size_t bitmaskSize, std::string_view name);
			Tag(Tag&& other) noexcept;

			std::string_view name() const { return _name; }

			Tag& operator = (Tag&& other) noexcept;

		private:
			Install const& _install;
			std::string_view _name;
			uint8_t _type;

			size_t _bitmaskSize;
			std::unique_ptr<uint8_t[]> _bitmask;
		};

		size_t size() const { return _entries.size(); }

	private:
		Install();

		struct Entry {
			Entry(Install const& install, io::IReadableStream& stream, size_t hashSize, std::string_view name);

		private:
			Install const& _install;

			std::string_view _name;
			size_t _fileSize;
			
			size_t _hashSize;
			std::unique_ptr<uint8_t[]> _hash;
		};

		std::list<std::string> _tagNames; // Actual storage for tag names
		std::unordered_map<std::string_view, Tag> _tags;

		std::list<std::string> _entryNames; // Actual storage for entry names
		std::unordered_map<std::string_view, Entry> _entries;
	};
}

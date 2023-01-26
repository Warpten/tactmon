#include "io/IStream.hpp"

#include <bit>
#include <filesystem>
#include <fstream>

namespace io {
	struct FileStream final : IReadableStream {
		FileStream(std::filesystem::path filePath, std::endian endianness);

	public: // IStream
		size_t GetLength() const override;
		operator bool() const { return _fileHandle.is_open(); }

	public: // IReadableStream
		size_t SeekRead(size_t offset) override;
		size_t GetReadCursor() const override;

	protected: // IReadableStream
		size_t _ReadImpl(std::span<std::byte> bytes) override;

	private:
		std::fstream _fileHandle;
		std::filesystem::path _filePath;
	};
}

#pragma once

#include "io/IStream.hpp"

#include <bit>
#include <cstdint>
#include <span>
#include <string>
#include <vector>

namespace io::mem {
	/**
	 * An implementation of the Stream interface.
	 */
	struct MemoryStream final : IReadableStream {
		MemoryStream(std::span<uint8_t> data, std::endian endianness);

		size_t GetReadCursor() const override { return _cursor; }
		size_t SeekRead(size_t offset) override;

	protected:
		size_t _ReadImpl(std::span<std::byte> writableSpan) override;

	private:
		std::span<uint8_t> _data;
		size_t _cursor = 0;
	};

	struct GrowableMemoryStream final : IReadableStream, IWritableStream {
		GrowableMemoryStream(std::span<uint8_t> data, std::endian endianness);

	public:
		size_t GetReadCursor() const override { return _readCursor; }
		size_t SeekRead(size_t offset) override;

	public:
		size_t GetWriteCursor() const override { return _writeCursor; }
		size_t SeekWrite(size_t offset) override;

	protected:
		size_t _ReadImpl(std::span<std::byte> writableSpan) override;
		size_t _WriteImpl(std::span<std::byte> writableSpan) override;

	private:
		std::vector<uint8_t> _data;
		size_t _readCursor = 0;
		size_t _writeCursor = 0;
	};
}
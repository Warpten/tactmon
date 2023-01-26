#include "io/mem/MemoryStream.hpp"

namespace io::mem {
	MemoryStream::MemoryStream(std::span<uint8_t> data, std::endian endianness)
		: IReadableStream(endianness), _data(data)
	{ }

	size_t MemoryStream::SeekRead(size_t offset) {
		_cursor = std::min(_data.size(), offset);

		return _cursor;
	}

	size_t MemoryStream::_ReadImpl(std::span<std::byte> bytes) {
		size_t length = std::min(bytes.size(), _data.size() - _cursor);

		bytes = std::span { reinterpret_cast<std::byte*>(_data.data()) + _cursor, length };
		_cursor += length;
		return length;
	}

	// ^^^ MemoryStream / GrowableMemoryStream vvv

	GrowableMemoryStream::GrowableMemoryStream(std::span<uint8_t> data, std::endian endianness)
		: IReadableStream(endianness), IWritableStream(endianness), _data(data.begin(), data.end())
	{ }


	size_t GrowableMemoryStream::SeekRead(size_t offset) {
		return _readCursor = std::min(offset, _data.size());
	}

	size_t GrowableMemoryStream::SeekWrite(size_t offset) {
		_writeCursor = offset;
		_data.resize(offset);
		return _writeCursor;
	}

	size_t GrowableMemoryStream::_ReadImpl(std::span<std::byte> bytes) {
		size_t length = std::min(bytes.size(), _data.size() - _readCursor);

		bytes = std::span{ reinterpret_cast<std::byte*>(_data.data()) + _readCursor, length };
		_readCursor += length;
		return length;
	}

	size_t GrowableMemoryStream::_WriteImpl(std::span<std::byte> bytes) {
		_data.resize(_writeCursor + bytes.size());

		std::memcpy(_data.data() + _writeCursor, bytes.data(), bytes.size());
		_writeCursor += bytes.size();
		return bytes.size();
	}
}

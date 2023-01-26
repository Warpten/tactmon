#include "io/fs/FileStream.hpp"

namespace io {
	FileStream::FileStream(std::filesystem::path filePath, std::endian endianness)
		: IReadableStream(endianness), _fileHandle(filePath.string().data(), std::ios::binary), _filePath(filePath) 
	{ }

	size_t FileStream::SeekRead(size_t offset) {
		_fileHandle.seekg(offset, std::ios::beg);
		return _fileHandle.tellg();
	}

	size_t FileStream::GetReadCursor() const {
		return const_cast<std::fstream&>(this->_fileHandle).tellg();
	}

	size_t FileStream::_ReadImpl(std::span<std::byte> bytes) {
		_fileHandle.read(reinterpret_cast<char*>(bytes.data()), bytes.size());
		return _fileHandle.gcount();
	}

	size_t FileStream::GetLength() const {
		return std::filesystem::file_size(_filePath);
	}
}
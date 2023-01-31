#pragma once

#include "io/IStream.hpp"

#include <filesystem>

#include <boost/iostreams/device/mapped_file.hpp>

namespace io {
    struct MemoryMappedFileStream final : IReadableStream {
        MemoryMappedFileStream(std::filesystem::path filePath, std::endian fileEndianness);

    public: // IStream
        size_t GetLength() const override;
        operator bool() const override { return _stream.is_open(); }

    public: // IReadableStream
        size_t SeekRead(size_t offset) override;
        size_t GetReadCursor() const override;
        void SkipRead(size_t offset) override { _cursor += offset; }
        bool CanRead(size_t count) const override { return _cursor + count <= _stream.size(); }

        std::byte const* Data() const override { return reinterpret_cast<std::byte const*>(_stream.data() + _cursor); }

    protected: // IReadableStream
        size_t _ReadImpl(std::span<std::byte> bytes) override;

    private:
        boost::iostreams::mapped_file_source _stream{ };
        size_t _cursor = 0;
    };
}

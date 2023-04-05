#pragma once

#include "libtactmon/io/IReadableStream.hpp"

#include <filesystem>

#include <boost/iostreams/device/mapped_file.hpp>

namespace libtactmon::io {
    struct FileStream final : IReadableStream {
        explicit FileStream(const std::filesystem::path& filePath);

    public: // IStream
        [[nodiscard]] std::size_t GetLength() const override;
        explicit operator bool() const override { return _stream.is_open(); }

    public: // IReadableStream
        size_t SeekRead(std::size_t offset) override;
        [[nodiscard]] std::size_t GetReadCursor() const override;
        void SkipRead(std::size_t offset) override { _cursor += offset; }
        [[nodiscard]] bool CanRead(std::size_t count) const override { return _cursor + count <= _stream.size(); }

        [[nodiscard]] std::span<std::byte const> Data() const override { return std::span { reinterpret_cast<std::byte const*>(_stream.data() + _cursor), _stream.size() - _cursor }; }

    protected: // IReadableStream
        std::size_t _ReadImpl(std::span<std::byte> bytes) override;

    private:
        boost::iostreams::mapped_file_source _stream { };
        std::size_t _cursor = 0;
    };
}

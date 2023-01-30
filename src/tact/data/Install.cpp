#include "tact/data/Install.hpp"
#include "io/IStream.hpp"

namespace tact::data {
    /* static */ std::optional<Install> Install::Parse(io::IReadableStream& stream) {
        if (!stream.CanRead(2 + 1 + 1 + 2 + 4))
            return std::nullopt;

        uint16_t signature = stream.Read<uint16_t>(std::endian::big);
        uint8_t version = stream.Read<uint8_t>(std::endian::little);
        uint8_t hashSize = stream.Read<uint8_t>(std::endian::little);
        uint16_t numTags = stream.Read<uint16_t>(std::endian::big);
        uint32_t numEntries = stream.Read<uint32_t>(std::endian::big);

        Install instance { };

        for (int i = 0; i < numTags; ++i) {
            std::string name;
            stream.ReadCString(name);
            std::string& nameItr = instance._tagNames.emplace_back(std::move(name));

            instance._tags.emplace(std::piecewise_construct,
                std::forward_as_tuple(std::string_view { nameItr }),
                std::forward_as_tuple(instance, stream, numEntries, std::string_view { nameItr })
            );
        }

        for (size_t i = 0; i < numEntries; ++i) {
            std::string name;
            stream.ReadCString(name);

            std::string& nameItr = instance._entryNames.emplace_back(std::move(name));

            instance._entries.emplace(std::piecewise_construct,
                std::forward_as_tuple(std::string_view { nameItr }),
                std::forward_as_tuple(instance, stream, hashSize, std::string_view { nameItr })
            );
        }

        return instance;
    }

    Install::Install() { }

    Install::Entry::Entry(Install const& install, io::IReadableStream& stream, size_t hashSize, std::string_view name)
        : _install(install)
    {
        _hashSize = hashSize;
        _hash = std::make_unique<uint8_t[]>(_hashSize);
        stream.Read(std::span { _hash.get(), hashSize }, std::endian::little);

        _fileSize = stream.Read<uint32_t>(std::endian::big);
    }

    Install::Tag::Tag(Install const& install, io::IReadableStream& stream, size_t bitmaskSize, std::string_view name) : _install(install) {
        _name = name;

        _type = stream.Read<uint16_t>(std::endian::big);
        _bitmaskSize = bitmaskSize;

        _bitmask = std::make_unique<uint8_t[]>((bitmaskSize + 7) / 8);
        stream.Read(std::span<uint8_t> { _bitmask.get(), (bitmaskSize + 7) / 8 }, std::endian::little);
    }

    Install::Tag::Tag(Tag&& other) noexcept
        : _install(other._install), _name(std::move(other._name)), _type(other._type), _bitmaskSize(other._bitmaskSize), _bitmask(std::move(other._bitmask))
    {
        other._bitmaskSize = 0;
    }

    Install::Tag& Install::Tag::operator = (Install::Tag&& other) noexcept {
        _name = std::move(other._name);
        _type = other._type;
        _bitmaskSize = other._bitmaskSize;
        _bitmask = std::move(other._bitmask);

        other._bitmaskSize = 0;
        return *this;
    }
}

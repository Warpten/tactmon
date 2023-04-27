#include "libtactmon/tact/data/Install.hpp"
#include "libtactmon/io/IReadableStream.hpp"
#include "libtactmon/Errors.hpp"

namespace libtactmon::tact::data {
    /* static */ Result<Install> Install::Parse(io::IReadableStream& stream) {
        if (!stream.CanRead(2 + 1 + 1 + 2 + 4))
            return Result<Install> { errors::tact::InvalidInstallFile("", "too small") };

        uint16_t signature = stream.Read<uint16_t>(std::endian::big);
        uint8_t version = stream.Read<uint8_t>();
        uint8_t hashSize = stream.Read<uint8_t>();
        uint16_t numTags = stream.Read<uint16_t>(std::endian::big);
        uint32_t numEntries = stream.Read<uint32_t>(std::endian::big);

        Install instance { };

        instance._tags.reserve(numTags);
        for (std::size_t i = 0; i < numTags; ++i) {
            std::string tagName;
            stream.ReadCString(tagName);

            instance._tags.emplace_back(stream, numEntries, std::move(tagName));
        }

        instance._entries.reserve(numEntries);
        for (std::size_t i = 0; i < numEntries; ++i) {
            std::string name;
            stream.ReadCString(name);

            instance._entries.emplace_back(stream, hashSize, name);
        }

        return Result<Install> { std::move(instance) };
    }

    Install::Install() = default;

    Install::Entry::Entry(io::IReadableStream& stream, std::size_t hashSize, std::string const& name)
        : _hash(stream.Data<uint8_t>().subspan(0, hashSize)), _name(name)
    {
        stream.SkipRead(hashSize);

        _fileSize = stream.Read<uint32_t>(std::endian::big);
    }

    Install::Tag::Tag(Tag const& other)
        : _bitmask(std::make_unique<uint8_t[]>(other._bitmaskSize))
    {
        _name = other._name;
        _bitmaskSize = other._bitmaskSize;
        _type = other._type;

        std::copy_n(other._bitmask.get(), _bitmaskSize, _bitmask.get());
    }

    Install::Tag& Install::Tag::operator = (Tag const& other) {
        _name = other._name;
        _bitmaskSize = other._bitmaskSize;
        _type = other._type;

        _bitmask = std::make_unique<uint8_t[]>(_bitmaskSize);
        std::copy_n(other._bitmask.get(), _bitmaskSize, _bitmask.get());

        return *this;
    }

    Install::Tag::Tag(io::IReadableStream& stream, std::size_t bitmaskSize, std::string&& name) : _name(std::move(name)) {
        _type = stream.Read<uint16_t>(std::endian::big);
        _bitmaskSize = bitmaskSize;

        _bitmask = std::make_unique<uint8_t[]>((bitmaskSize + 7) / 8);
        stream.Read(std::span<uint8_t> { _bitmask.get(), (bitmaskSize + 7) / 8 }, std::endian::little);
    }

    Install::Tag::Tag(Tag&& other) noexcept
        : _name(other._name), _type(other._type), _bitmaskSize(other._bitmaskSize), _bitmask(std::move(other._bitmask))
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

    std::optional<tact::CKey> Install::FindFile(std::string_view fileName) const {
        auto itr = std::find_if(_entries.begin(), _entries.end(), [&](Entry const& entry) {
            return entry.name() == fileName;
        });

        if (itr != _entries.end())
            return itr->ckey();

        return std::nullopt;
    }

    bool Install::Tag::Matches(std::size_t fileIndex) const {
        return false;

        // Buggy, disabled for now.
        // std::size_t byteIndex = fileIndex / sizeof(uint8_t);
        // std::size_t bitIndex = fileIndex % sizeof(uint8_t);
        //
        // return (_bitmask[byteIndex] & (1 << bitIndex)) != 0;
    }
}

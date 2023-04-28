<font size="7">
⚠️ None of the code in this repository has been tested in production. Use at your own risk. ⚠️

⚠️ You **especially** should not see the code powering the database access used by `tactman`.⚠️
</font>

<hr />

# tactmon

A CDN tracker for Blizzard products.

# Features

1. ✔️ Ribbit
2. ✔️ TACT data and products.<br/>Partial support: Install tags are **not** supported.
3. World of Warcraft game products support.

## Dependencies

|                | `libtactmon` | `tactmon` |
|----------------|--------------|-----------|
| `Boost`        | ✔️ | ✔️ |
| `spdlog`       | ✔️ | ✔️ |
| `zlib`         | ✔️ | ❌ |
| `openssl`      | ✔️ | ❌ |
| `dpp`          | ❌ | ✔️ |
| `libpqxx`      | ❌ | ✔️ |

## Compiling

Using [vcpkg](https://vcpkg.io) and [CMake](https://www.cmake.org) is recommended.

### Windows
```
git clone https://github.com/Warpten/tactmon.git
cd tactmon
cmake -S .. -DCMAKE_TOOLCHAIN_FILE="${VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake"
```

If you want to load up the library in Visual Studio, the example above will generate a solution with no include paths; VCPKG generates extra compiler arguments through the command line that corresponds to the actual include paths. IntelliSense picks these up, but VA-X (at the time of writing) does not, making IDE autocompletion non-functional if IntelliSense is disabled (which it should, if you have VA-X). In that case, add `-DVCPKG_MANIFEST_MODE=OFF` to the command line invocation above.

### Linux
```
git clone https://github.com/Warpten/tactmon.git
cmake --preset ninja-multiconfiguration-vcpkg
cmake --build --preset ninja-multiconfiguration-vcpkg --config Release
```

## libtactmon

`libtactmon` provides utilities to monitor TACT content delivery by Blizzard, as well as Ribbit services. Every utility type exposed by libtactmon lives in the `libtactmon` namespace.

#### `ribbit::CDNs<Region, Version>`

Emits a request of the form `{Version}/products/{product}/cdns` to the Ribbit endpoint `{Region}.version.battle.net:1119`. This executor expects a string argument corresponding to the name of the product to query. If version is not specified, `v1` is the default.

```cpp
#include <libtactmon/ribbit/Commands.hpp>
#include <libtactmon/ribbit/types/CDNs.hpp>

namespace ribbit = libtactmon::ribbit;

std::optional<ribbit::types::CDNs> response = ribbit::CDNs<net::ribbit::Region::EU>::Execute("wow");
```

### `ribbit::Versions<Region, Version>`

Emits a request of the form `{Version}/products/{product}/versions` to the Ribbit endpoint `{Region}.version.battle.net:1119`. This executor expects a string argument corresponding to the name of the product to query. If version is not specified, `v1` is the default.

```cpp
#include <libtactmon/ribbit/Commands.hpp>
#include <libtactmon/ribbit/types/Versions.hpp>

namespace ribbit = libtactmon::ribbit;

std::optional<ribbit::types::Versions> response = ribbit::Versions<net::ribbit::Region::EU>::Execute("wow");
```

### `ribbit::Summary<Region, Version>`

Emites a request of the form `{Version}/summary` to the Ribbit endpoint `{Region}.version.battle.net:1119`. If version is not specified, `v1` is the default.

```cpp
#include <libtactmon/ribbit/Commands.hpp>
#include <libtactmon/ribbit/types/Summary.hpp>

namespace ribbit = libtactmon::ribbit;

std::optional<ribbit::types::Summary> response = ribbit::Summary<net::ribbit::Region::EU>::Execute();
```

### `tact::data::product::Product`

This is the basic implementation of a game-agnostic product. Construction of this object requires the name of the product as well as an instance of `tact::Cache` that will behave as a local cache of the configuration and data files available on Blizzard CDNs.

1. `bool Product::Load(std::string_view buildConfig, std::string_view cdnConfig)`

Loads a specific configuration. This function also emits a request to Ribbit endpoint `v1/products/{product}/cdns` to acquire up-to-date CDNs servers. If the files exists in the local cache, no HTTP request to the CDNs is emitted. Encoding and install manifests are downloaded and loaded, as well as archive indices.

2. `std::optional<tact::data::FileLocation> Product::FindFile(std::string_view fileName) const`

Returns the location of a file in the currently loaded configuration, or an empty optional if said file could not be found. The base implementation only searches in the install manifest.

3. `std::optional<tact::data::FileLocation> Product::FindFile(uint32_t fileDataID) const`

Returns the location of a file in the currently loaded configuration, or an empty optional if said file could not be found.

:information_source: File ID search is usually provided by the root manifest, which this type does not load, as its format is often product-specific; product-specific implementations of this type ought to override this method.

4. `std::optional<tact::data::FileLocation> Product::FindFile(tact::CKey const& contentKey) const`

Returns the location of a file, identified by its content key, in the currently loaded configuration, or an empty optional if said file could not be found. This particular overload searches the encoding manifest for the given content key.

5. `std::optional<tact::data::ArchiveFileLocation> Product::FindArchive(tact::EKey const& ekey) const`

Returns the archive containing a file, identified by its encoding key. This function is used in conjunction with one of the `FindFile` overloads:

```cpp
namespace tact = libtactmon::tact;

std::optional<tact::data::FileLocation> location = product.FindFile("Wow.exe");
if (location.has_value()) {
    for (size_t i = 0; i < location->keyCount(); ++i) {
        std::optional<tact::data::ArchiveFileLocation> archiveLocation = product.FindArchive((*location)[i]);
        if (archiveLocation.has_value()) {
            // Do things with the archive location; generate link or whatever, or just download it.
        }
    }
}
```

:information_source: Depending on the build configuration of the product you're trying to process, this function **may** return an empty optional even if the file exists. Later versions of TACT configuration files include an index for files that live outside of archives (due to their size, usually), allowing this function to return correctly; older versions however do not provide such an index and you're left to assume that you can access the file directly through its encoding key.

### `tact::data::product::wow::Product`

A specialization of `tact::data::product::Product` tailored for CDN installations of various World of Warcraft products.

### `tact::data::FileLocation`

Describes the location of a file.

1. `size_t FileLocation::fileSize() const`

Returns the decompressed size of a file.

2. `size_t FileLocation::keyCount() const`

Returns the amount of encoding keys that represent this file.

3. `std::span<const uint8_t> FileLocation::operator [] (size_t index) const`

Returns the `n`-th key that represents this file.

### 'tact::data::ArchiveFileLocation`

Describes the location of a file in a CDN archive.

1. `size_t ArchiveFileLocation::fileSize() const`

Returns the compressed size of the file within the archive.

2. `size_t ArchiveFileLocation::offset() const`

Returns the location of the file within its archive.

3. `std::string ArchiveFileLocation::name() const`

Returns the name of the archive containing the file.

### `tact::data::product::ResourceResolver`

An utility type providing methods to resolve files from Blizzard CDNs.

1. `Result<io::FileStream> ResourceResolver::ResolveConfiguration(ribbit::types::CDNs const& cdns, std::string_view key) const;`

Returns a stream to a local copy of a configuration file, downloading it from Blizzard CDNs if necessary.

2. `Result<io::FileStream> ResourceResolver::ResolveData(ribbit::types::CDNs const& cdns, std::string_view key) const;`

Returns a stream to a local copy of a data file, downloading it from Blizzard CDNs if necessary.

3. `Result<io::GrowableMemoryStream> ResourceResolver::ResolveBLTE(ribbit::types::CDNs const& cdns, tact::EKey const& encodingKey, tact::CKey const& contentKey) const;`

Returns a stream to a decompressed in-memory version of a BLTE data file, downloading it from Blizzard CDNs if necessary.


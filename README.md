<font size="7">
⚠️ None of the code in this repository has been tested in production. Use at your own risk. ⚠️
    
⚠️ You **especially** should not see the code powering the database access used by `tactman`.⚠️
</font>

<hr />

# tactmon

A CDN tracker for Blizzard products. Has technically support for more than just World of Warcraft, but the intended audience being [TrinityCore](http://github.com/TrinityCore), this is what you get.

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

`libtactmon` provides utilities to monitor TACT content delivery by Blizzard, as well as Ribbit services.

### `net::ribbit::CDNs<Region, Version>`

Emits a request of the form `{Version}/products/{product}/cdns` to the Ribbit endpoint `{Region}.version.battle.net:1119`. This executor expects a string argument corresponding to the name of the product to query.

```cpp
#include <libtactmon/ribbit/Commands.hpp>
#include <libtactmon/ribbit/types/CDNs.hpp>

namespace ribbit = libtactmon::ribbit;

std::optional<ribbit::types::CDNs> response = ribbit::CDNs<net::ribbit::Region::EU>::Execute("wow");
```

### `net::ribbit::Versions<Region, Version>`

Emits a request of the form `{Version}/products/{product}/versions` to the Ribbit endpoint `{Region}.version.battle.net:1119`. This executor expects a string argument corresponding to the name of the product to query.

```cpp
#include <libtactmon/ribbit/Commands.hpp>
#include <libtactmon/ribbit/types/Versions.hpp>

namespace ribbit = libtactmon::ribbit;

std::optional<ribbit::types::Versions> response = ribbit::Versions<net::ribbit::Region::EU>::Execute("wow");
```

### `net::ribbit::Summary<Region, Version>`

Emites a request of the form `{Version}/summary` to the Ribbit endpoint `{Region}.version.battle.net:1119`.

```cpp
#include <libtactmon/ribbit/Commands.hpp>
#include <libtactmon/ribbit/types/Summary.hpp>

namespace ribbit = libtactmon::ribbit;

std::optional<ribbit::types::Summary> response = ribbit::Summary<net::ribbit::Region::EU>::Execute();
```

### `libtactmon::tact::data::product::Product`

This is the basic implementation of a game-agnostic product. Construction of this object requires the name of the product as well as an instance of `tact::Cache` that will behave as a local cache of the configuration and data files available on Blizzard CDNs.

1. `bool Product::Load(std::string_view buildConfig, std::string_view cdnConfig)`

Loads a specific configuration. This function also emits a request to Ribbit endpoint `v1/products/{product}/cdns` to acquire up-to-date CDNs servers. If the files exists in the local cache, no HTTP request to the CDNs is emitted. Encoding and install manifests are downloaded and loaded, as well as archive indices.

2. `std::optional<libtactmon::tact::data::FileLocation> Product::FindFile(std::string_view fileName) const`

Returns the location of a file in the currently loaded configuration, or an empty optional if said file could not be found. The base implementation only searches in the install manifest.

3. `std::optional<libtactmon::tact::data::FileLocation> Product::FindFile(uint32_t fileDataID) const`

Returns the location of a file in the currently loaded configuration, or an empty optional if said file could not be found. File ID search is usually provided by the root manifest, which this type does not load, as its format is often product-specific; product-specific implementations of this type ought to override this method.

4. `std::optional<libtactmon::tact::data::FileLocation> Product::FindFile(tact::CKey const& contentKey) const`

Returns the location of a file in the currently loaded configuration, or an empty optional if said file could not be found. This particular overload searches the encoding manifest for the given content key.

5. `std::optional<libtactmon::tact::data::ArchiveFileLocation> Product::FindArchive(tact::EKey const& ekey) const`

Returns the location of a given encoding key in the archives of the loaded configuration. This function is used in conjunction with one of the `FindFile` overloads:

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

### `libtactmon::tact::data::product::wow::Product`

This is a specialization of `libtactmon::tact::data::product::Product` tailored for CDN installations of various World of Warcraft products.

### `libtactmon::tact::data::FileLocation`

Describes the location of a file.

1. `size_t FileLocation::fileSize() const`

Returns the decompressed size of a file.

2. `size_t FileLocation::keyCount() const`

Returns the amount of encoding keys that represent this file.

3. `std::span<const uint8_t> FileLocation::operator [] (size_t index) const`

Returns the `n`-th key that represents this file.

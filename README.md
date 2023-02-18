<font size="7">
⚠️ None of the code in this repository has been tested in production. Use at your own risk. ⚠️
    
⚠️ You **especially** should not see the code powering the database access used by `tactman`.⚠️
</font>

<hr />

# tactmon

A CDN tracker for Blizzard products. Has technically support for more than just World of Warcraft, but the intended audience being [TrinityCore](http://github.com/TrinityCore), this is what you get.

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

### Windows

Build requires CMake. `Boost`, `dpp`, `openssl`, `zlib`, `spdlog` and `libpqxx` must be available to find by CMake; this can be done through `vcpkg` by running the following commands:

```
vcpkg install dpp:x64-windows
vcpkg install pqxx:w64-windows
vcpkg install libpqxx:x64-windows
vcpkg install openssl:x64-windows
vcpkg install zlib:x64-windows
vcpkg install spdlog:x64-windows
```

Then, make sure to run CMake with the `CMAKE_TOOLCHAIN_FILE` set to `$(VCPKG_INSTALL_ROOT)/scripts/buildsystems/vcpkg.cmake`. If all you're building is `libtactmon`, your only requirements are `zlib`, `spdlog`, `Boost` and `openssl`.

### Linux

To be redacted.

## libtactmon

`libtactmon` provides utilities to monitor TACT content delivery by Blizzard, as well as Ribbit services.

### `net::ribbit::CDNs<Region, Version>`

Emits a request of the form `{Version}/products/{product}/cdns` to the Ribbit endpoint `{Region}.version.battle.net:1119`. This executor expects a string argument corresponding to the name of the product to query.

```cpp
#include <net/ribbit/Commands.hpp>
#include <net/ribbit/types/CDNs.hpp>

net::ribbit::CDNs<net::ribbit::Region::EU, net::ribbit::Version::V1> executor;
std::optional<net::ribbit::types::CDNs> response = executor("wow");
```

### `net::ribbit::Versions<Region, Version>`

Emits a request of the form `{Version}/products/{product}/versions` to the Ribbit endpoint `{Region}.version.battle.net:1119`. This executor expects a string argument corresponding to the name of the product to query.

```cpp
#include <net/ribbit/Commands.hpp>
#include <net/ribbit/types/Versions.hpp>

net::ribbit::Versions<net::ribbit::Region::EU, net::ribbit::Version::V1> executor;
std::optional<net::ribbit::types::Versions> response = executor("wow");
```

### `net::ribbit::Summary<Region, Version>`

Emites a request of the form `{Version}/summary` to the Ribbit endpoint `{Region}.version.battle.net:1119`.

```cpp
#include <net/ribbit/Commands.hpp>
#include <net/ribbit/types/Summary.hpp>

net::ribbit::Summary<net::ribbit::Region::EU, net::ribbit::Version::V1> executor;
std::optional<net::ribbit::types::Summary> response = executor();
```

### `tact::data::product::Product`

This is the basic implementation of a game-agnostic product. Construction of this object requires the name of the product as well as an instance of `tact::Cache` that will behave as a local cache of the configuration and data files available on Blizzard CDNs.

1. `bool Product::Load(std::string_view buildConfig, std::string_view cdnConfig)`

Loads a specific configuration. This function also emits a request to Ribbit endpoint `v1/products/{product}/cdns` to acquire up-to-date CDNs servers. If the files exists in the local cache, no HTTP request to the CDNs is emitted. Encoding and install manifests are downloaded and loaded, as well as archive indices.

2. `std::optional<tact::data::FileLocation> Product::FindFile(std::string_view fileName) const`

Returns the location of a file in the currently loaded configuration, or an empty optional if said file could not be found. The base implementation only searches in the install manifest.

3. `std::optional<tact::data::FileLocation> Product::FindFile(uint32_t fileDataID) const`

Returns the location of a file in the currently loaded configuration, or an empty optional if said file could not be found. File ID search is usually provided by the root manifest, which this type does not load, as its format is often product-specific; product-specific implementations of this type ought to override this method.

4. `std::optional<tact::data::FileLocation> Product::FindFile(tact::CKey const& contentKey) const`

Returns the location of a file in the currently loaded configuration, or an empty optional if said file could not be found. This particular overload searches the encoding manifest for the given content key.

5. `std::optional<tact::BLTE> Product::Open(tact::data::FileLocation const& location) const`

Downloads to the local cache a given file, as specified by its location returned by one of the `FindFile` overloads. Returns a decompressed stream.

6. `std::optional<tact::data::ArchiveFileLocation> Product::FindArchive(tact::EKey const& ekey) const`

Returns the location of a given encoding key in the archives of the loaded configuration. This function is used in conjunction with one of the `FindFile` overloads:

```cpp
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

### `tact::data::product::wow::Product`

This is a specialization of `tact::data::product::Product` tailored for CDN installations of various World of Warcraft products.


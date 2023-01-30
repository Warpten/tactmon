## wow-build-monitor

A CDN tracker for Blizzard products. Has technically support for more than just World of Warcraft, but the intended audience being [TrinityCore](http://github.com/TrinityCore), this is what you get.

### Features

| Ribbit command               | Supported? | Ribbit command               | Supported? |
|------------------------------|------------|------------------------------|------------|
| `v1/summary`                 | ✔️ | `v2/summary`                | ✔️ |
| `v1/products/___/versions`  | ✔️ | `v2/products/___/versions`  | ✔️ |
| `v1/products/___/cdns`      | ✔️ | `v2/products/___/cdns`      | ✔️ |
| `v1/products/___/bgdl`      | ✔️ | `v2/products/___/bgdl`      | ✔️ |
| `v1/certs/___`              | ❌ |
| `v1/ocsp/___`               | ❌ |
| `v1/ext`                    | ❌ |

### API

1. `tact::data::product::Product`

This type provides basic functions for interacting with a specific product. Implementations for a specific product live in `tact::data::product::${productName}::Product`. Currently, only World of Warcraft products are supported.

2. `net::ribbit::CommandExecutor<C, R, V>`

This types emit a Ribbit request with the given parameters, where:
   - `C` identifies the command being sent out.
   - `R` identifies the region being queried.
   - `V` identifies the version of the endpoint to it.

### Example

```cpp
#include <tact/data/product/wow/Product.hpp>

void Execute(boost::asio::io_context& context) {
    tact::data::product::wow::Product wow("wow", context);
    wow.Refresh();
    
    std::optional<tact::data::FileLocation> fileLocation = wow.FindFile("WoW-64.exe"); // or wow.FindFile(some_fdid)
    if (fileLocation.has_value()) {
        std::optional<tact::BLTE> fileStream = wow.Open(fileLocation);
        if (fileStream.has_value())
            fileStream->SaveToDisk("WoW-64.exe");
    }
}
```

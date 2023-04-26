#include "libtactmon/Errors.hpp"

#include <string>

namespace { // Force internal linkage
    struct ErrorsCategory : std::error_category {
        const char* name() const noexcept override { return "libtactmon"; }
        std::string message(int ev) const noexcept override;
    };

    std::string ErrorsCategory::message(int ev) const noexcept {
        switch (static_cast<libtactmon::Error>(ev)) {
        case libtactmon::Error::ResourceResolutionFailed:
            return "resource resolution failed";
        }

#ifdef __GNUC__ // GCC, Clang, ICC
        __builtin_unreachable();
        #elifdef _MSC_VER // MSVC
            __assume(false);
#endif
        return "(unknown error)";
    }

    const ErrorsCategory errorsCategory{ };
}

namespace libtactmon {
    std::error_code make_error_code(Error err) {
        return { static_cast<int>(err), errorsCategory };
    }
}

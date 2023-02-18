#pragma once

#include <algorithm>
#include <string>

namespace ext {
    template <size_t N>
    struct Literal {
        constexpr Literal(const char(&val)[N]) {
            std::copy_n(val, N, Value);
        }

        // TODO: Clang doesn't like this being constexpr ?!
        std::string ToString() const { return Value; }

        constexpr static const size_t Size = N;
        char Value[N];
    };
}

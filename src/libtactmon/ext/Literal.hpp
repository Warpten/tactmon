#pragma once

#include <algorithm>

namespace ext {
    template <size_t N>
    struct Literal {
        constexpr Literal(const char(&val)[N]) {
            std::copy_n(val, N, Value);
        }

        constexpr static const size_t Size = N;
        char Value[N];
    };
}

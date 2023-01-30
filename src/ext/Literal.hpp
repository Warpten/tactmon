#pragma once

namespace ext {
    template <size_t N>
    struct Literal {
        constexpr Literal(char(&val)[N]) {
            std::copy_n(val, N, Value);
        }

        char Value[N];
    };
}
#pragma once

#include <algorithm>
#include <string>

namespace utility {
    template <size_t N>
    struct Literal {
        constexpr Literal(const char(&val)[N]) {
            std::copy_n(val, N, Value);
        }

        std::string ToString() const { return Value; }

        constexpr static const size_t Size = N;
        char Value[N];
    };

    namespace concepts {
        namespace detail {
            template <size_t N>
            static consteval bool is_literal(Literal<N>) { return true; }
            static consteval bool is_literal(...) { return false; }
        }

        template <auto V>
        concept IsLiteral = detail::is_literal(V);
    }
}

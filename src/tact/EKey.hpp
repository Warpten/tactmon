#pragma once

#include <array>
#include <cstdint>
#include <optional>
#include <span>
#include <string_view>

namespace tact {
    struct EKey {
        explicit EKey(std::string_view value) noexcept;
        EKey() = default;

        std::span<const uint8_t> Value() const { return std::span{ _value }; }

        std::string ToString() const;

    private:
        std::array<uint8_t, 16> _value = { };
    };
}
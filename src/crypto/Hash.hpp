#pragma once

#include <array>
#include <concepts>
#include <cstdint>
#include <span>
#include <type_traits>

#include <openssl/evp.h>

namespace crypto {
    struct HashImpl {
        using Creator = EVP_MD const* (*)();

        static EVP_MD_CTX* MakeContext() noexcept { return EVP_MD_CTX_new(); }
        static void DestroyContext(EVP_MD_CTX* context) { EVP_MD_CTX_free(context); }
    };

    template <HashImpl::Creator Creator, size_t Size>
    struct Hash {
        constexpr static const size_t DigestLength = Size;
        using Digest = std::array<uint8_t, Size>;

        template <typename... Ts>
        // requires requires ((std::is_same_v<decltype(Hash{}.UpdateData(Ts{})), Digest > && ...))
        static Digest Of(Ts&&... pack) {
            Hash hash{ };
            (hash.UpdateData(pack), ...);
            hash.Finalize();
            return hash.GetDigest();
        }

        explicit Hash() noexcept : _context(HashImpl::MakeContext()) {
            uint32_t result = EVP_DigestInit_ex(_context, Creator(), nullptr);
        }

        Hash(Hash const& other) : _context(HashImpl::MakeContext()) {
            *this = other;
        }

        Hash(Hash&& other) noexcept {
            *this = std::move(other);
        }

        Hash& operator = (Hash const& other) {
            if (this == &other)
                return *this;

            uint32_t result = EVP_MD_CTX_copy_ex(_context, other._context);
            _digest = other._digest;
            return *this;
        }

        Hash& operator = (Hash&& other) {
            _context = std::exchange(other._context, HashImpl::Creator());
            _digest = std::exchange(other._digest, Digest{ });

            return *this;
        }

        void UpdateData(std::span<uint8_t const> data) {
            EVP_DigestUpdate(_context, data.data(), data.size());
        }

        template <typename T>
        void UpdateData(std::span<T const> data) {
            if constexpr (std::is_same_v<T, std::byte>) {
                EVP_DigestUpdate(_context, reinterpret_cast<uint8_t*>(data.data()), data.size());
            }
            else {
                UpdateData(std::as_bytes(data));
            }
        }

        void UpdateData(std::string_view data) {
            EVP_DigestUpdate(_context, reinterpret_cast<uint8_t*>(data.data()), data.size());
        }

        void UpdateData(std::string const& data) {
            EVP_DigestUpdate(_context, data.data(), data.size());
        }

        template <size_t N>
        void UpdateData(std::array<uint8_t const, N> data) {
            EVP_DigestUpdate(_context, data.data(), N);
        }

        template <typename T>
        requires std::integral<T>
        void UpdateData(T value) {
            EVP_DigestUpdate(_context, std::addressof(value), sizeof(T));
        }

        void Finalize() {
            uint32_t length = 0;
            EVP_DigestFinal_ex(_context, _digest.data(), &length);
        }

        Digest const& GetDigest() const { return _digest; }

    private:
        EVP_MD_CTX* _context;
        Digest _digest;
    };

    using MD5 = Hash<EVP_md5, 16>;
    using SHA1 = Hash<EVP_sha1, 20>;
    using SHA256 = Hash<EVP_sha256, 32>;
}

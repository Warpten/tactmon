#pragma once

#include "libtactmon/Errors.hpp"

#include <optional>
#include <type_traits>
#include <variant>

namespace libtactmon {
    struct Success { };
    struct Failure { };

    namespace dtl {
        template <typename T, typename...> struct convertible : std::false_type { };
        template <typename T, typename U> struct convertible<T, U> : std::is_convertible<T, U> { };
    }

    /**
     * A type that represents either success or failure.
     *
     * @tparam[in] R The type of the value possibly stored in this result.
     * @tparam[in] E The type of the error value possibly stored in this result.
     */
    template <typename R, typename E = errors::Error>
    struct Result final {
        static_assert(!std::is_void_v<R>, "R must not be void");
        static_assert(!std::is_void_v<E>, "E must not be void");

        // Construct R from Ts...
        // - Enabled if E is not constructible from Ts
        // - Enabled if R is not convertible from Ts (where sizeof...(Ts) == 1)
        // - Enabled if E is not convertible from Ts (where sizeof...(Ts) == 1)
        template <
            typename... Ts,
            typename = std::enable_if_t<
                std::is_constructible_v<R, Ts...>
                && !std::is_constructible_v<E, Ts...>
                && !dtl::convertible<R, Ts...>::value // Special handling to avoid compiler errors due to sizeof...(Ts) > 1 for SFINAE
                && !dtl::convertible<E, Ts...>::value
            >
        > explicit Result(Ts&&... args) : _result(std::in_place_index<0>, std::forward<Ts>(args)...) { }

        // Construct from value
        // - Enabled if R != E
        template <typename X>
        explicit Result(X&& value, std::enable_if_t<std::is_same_v<X, R> && !std::is_same_v<R, E>, float> = 0.0f)
            : _result(std::in_place_index<0>, std::forward<X>(value))
        { }

        // Construct from error
        // - Enabled if R != E
        template <typename X>
        explicit Result(X&& e, std::enable_if_t<std::is_same_v<X, E> && !std::is_same_v<R, E>, int> = 0)
            : _result(std::in_place_index<1>, std::forward<X>(e))
        { }

        // Construct R from Ts...
        // - Enabled if R is constructible from Ts...
        // - Enabled if E is constructible from Ts...
        template <typename... Ts, typename = std::enable_if_t<std::is_constructible_v<R, Ts...> && std::is_constructible_v<E, Ts...>>>
        explicit Result(Success, Ts&&... args)
            : _result(std::in_place_index<0>, R{ std::forward<Ts>(args)... }) { }

        // Construct E from Ts...
        // - Enabled if R is constructible from Ts...
        // - Enabled if E is constructible from Ts...
        template <typename... Ts, typename = std::enable_if_t<std::is_constructible_v<R, Ts...> && std::is_constructible_v<E, Ts...>>>
        explicit Result(Failure, Ts&&... args)
            : _result(std::in_place_index<1>, E{ std::forward<Ts>(args)... }) { }

        ~Result() = default;

        Result(Result&& other) noexcept : _result(std::move(other._result)) { }

        Result& operator = (Result&& other) noexcept {
            _result = std::move(other._result);
            return *this;
        }

        Result(Result const& other) = delete;
        Result& operator = (Result const&) = delete;

    public:
        explicit operator bool() const { return has_value(); }

        /**
         * Tests wether this @p Result holds an ok value.
         */
        bool has_value() const { return _result.index() == 0; }

        /**
         * Tests wether this @p Result holds an error value.
         */
        bool has_error() const { return _result.index() == 1; }

        /**
         * Returns a new @p Result where the result type is the outcome of applying a function to the stored result of the current @p Result.
         * The eventual error code is carried over untouched.
         *
         * If you know functional programming, this is a map bind.
         * This is a terminal operation; the current object is considered moved-from after this call.
         *
         * @param[in] handler A function that accepts the result type if this @p Result and returns a new type.
         */
        template <typename Transform>
        auto and_then(Transform handler) && -> Result<std::invoke_result_t<Transform, R&&>, E> {
            using value_type = std::invoke_result_t<Transform, R&&>;
            using result_type = Result<value_type, E>;

            if (!has_value()) {
                if constexpr (std::is_same_v<value_type, E>) {
                    return result_type{ Failure { }, std::move(*this).code() };
                } else {
                    return result_type{ std::move(*this).code() };
                }
            }

            if constexpr (std::is_same_v<value_type, R>) {
                return result_type{ Success { }, handler(std::move(*this).unwrap()) };
            } else {
                return result_type{ handler(std::move(*this).unwrap()) };
            }
        }

        /**
         * Applies a function to the current @p Result, changing the value (and possibly the type) stored. If there is no stored
         * value, the error value is carried over.
         *
         * If you know functional programming, this is a functor bind.
         * This is a terminal operation; the current object is considered moved-from after this call.
         *
         * @param[in] handler A function that accepts the result type if this @p Result and returns a new @p Result value (of a possibly new type).
         */
        template <typename Transform>
        auto transform(Transform handler) && -> std::invoke_result_t<Transform, R&&> {
            using result_type = std::invoke_result_t<Transform, R&&>;

            if (!has_value())
                return result_type{ std::move(*this).code() };

            return result_type{ handler(std::move(*this).unwrap()) };
        }

        /**
         * Applies either of the provided functions to the current @p Result, depending on wether or not a value or an error value is held.
         *
         * This is a terminal operation; the current object is considered moved-from after this call.
         *
         * @param[in] successHandler A function accepting the value stored in this @p Result.
         * @param[in] failureHandler A function accepting the error code stored in this @p Result.
         */
        template <typename MapSuccess, typename MapFailure>
        auto then(MapSuccess successHandler, MapFailure failureHandler) && -> void {
            if (!has_value())
                failureHandler(std::move(*this).code());
            else
                successHandler(std::move(*this).unwrap());
        }

        /**
         * Returns the contained ok value.
         *
         * This is a terminal operation; the current object is considered moved-from after this call.
         *
         * Because this function may panic, its use is generally discouraged. Prefer @ref unwrap_or_else or @ref unwrap_or_default.
         */
        auto unwrap() && -> R { return std::get<0>(std::move(_result)); }

        /**
         * Returns the contained ok value, or computes it from the provided close.
         *
         * This is a terminal operation; the current object is considered moved-from after this call.
         *
         * @param[in] f A closure returning an instance of an ok value.
         */
        template <typename F, typename = std::enable_if_t<std::is_same_v<std::invoke_result_t<F>, R>>>
        auto unwrap_or_else(F f) && -> R {
            return !has_value() ? f() : std::move(*this).unwrap();
        }

        /**
         * Returns the contained ok value, or the default value.
         *
         * This is a terminal operation; the current object is considered moved-from after this call.
         */
        template <typename U = R, typename = std::enable_if_t<std::is_default_constructible_v<R>>>
        auto unwrap_or_default() && -> R {
            return !has_value() ? R{ } : std::move(*this).unwrap();
        }

        E const& code() const& { return std::get<1>(_result); }
        E&& code() && { return std::get<1>(std::move(_result)); }

        /**
         * Transfers this @p Result into an optional.
         *
         * This is a terminal operation; the current object is considered moved-from after this call.
         */
        std::optional<R> ToOptional() && {
            return std::optional<R> { std::get<0>(std::move(_result)) };
        }

        /**
         * Provides read-only access to the value stored in this @p Result. This function assumes that this @p Result
         * is not currently holding an error value.
         *
         * This is not a terminal operation. Use with caution.
         */
        R const* operator -> () const { return std::addressof(std::get<0>(_result)); }

    private:
        std::variant<R, E> _result;
    };
}

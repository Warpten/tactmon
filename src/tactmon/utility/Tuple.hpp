#pragma once

/**
 * Provides a non-recursive implementation of a tuple.
 */

#include <array>
#include <type_traits>
#include <utility>

namespace utility {
    template <std::size_t N> struct Constant { constexpr static const auto value = N; };

    /**
    * Forward declarations.
    */
    template <typename... Ts> class tuple;
    template <typename T> struct tuple_size;

    namespace detail {
        /**
        * Stores a value of type T.
        */
        template <std::size_t I, typename T>
        struct tuple_base_item {
            tuple_base_item() : value_() { }
            explicit tuple_base_item(T&& value) : value_(value) { }

            tuple_base_item(tuple_base_item const&) = default;
            tuple_base_item(tuple_base_item&&) noexcept = default;

            tuple_base_item& operator = (tuple_base_item const&) = default;
            tuple_base_item& operator = (tuple_base_item&&) = default;

            constexpr static const std::size_t index = I;
            using value_type = T;

            T value_;
        };

        /**
        * Actual implementation of a tuple. The first template type is an index sequence, and the second one
        * is the individual storage types for values.
        */
        template <typename, typename...> struct tuple_base;

        template <std::size_t... Is, typename... Ts>
        struct tuple_base<std::index_sequence<Is...>, Ts...> : tuple_base_item<Is, Ts>... {
            tuple_base() : tuple_base_item<Is, Ts>()... { }
            explicit tuple_base(Ts&&... args) : tuple_base_item<Is, Ts>(std::forward<Ts>(args))... { }

            tuple_base(tuple_base const&) = default;
            tuple_base(tuple_base&&) = default;
            tuple_base& operator=(tuple_base const& rhs) = default;
            tuple_base& operator=(tuple_base&&) = default;
        };

        template <>
        struct tuple_base<std::index_sequence<>> {
            tuple_base() { }

            tuple_base(tuple_base const&) = default;
            tuple_base(tuple_base&&) = default;
            tuple_base& operator=(tuple_base const& rhs) = default;
            tuple_base& operator=(tuple_base&&) = default;
        };

        /**
        * Helper trait to retrieve the types of an element of a tuple at a given offset.
        */
        class select_base_by_index final {
        public:
            template <std::size_t I, typename T>
            static auto select_base_impl(tuple_base_item<I, T>) -> detail::tuple_base_item<I, T>;
            template <std::size_t I, typename T>
            static auto select_impl(tuple_base_item<I, T>) -> T;

            template <std::size_t I, typename... Ts>
            static auto select_(tuple<Ts...> t) {
                return select_impl<I>(t);
            };
            template <std::size_t I, typename... Ts>
            static auto select_base_(tuple<Ts...> t) {
                return select_base_impl<I>(t);
            }

        public:
            template <std::size_t I, typename Tuple>
            using type = decltype(select_<I>(std::declval<std::decay_t<Tuple>>()));
            template <std::size_t I, typename Tuple>
            using base_type = decltype(select_base_<I>(std::declval<std::decay_t<Tuple>>()));
        };

        /**
        * Helper trait to slice elements of a tuple by a specific value.
        */
        class select_base_by_type final {
        public:
            template <typename T, std::size_t I>
            static auto select_base_impl(tuple_base_item<I, T>) -> detail::tuple_base_item<I, T>;
            template <typename T, std::size_t I> static auto select_impl(tuple_base_item<I, T>) -> T;

            template <typename T, typename... Ts>
            static auto select_(tuple<Ts...> t) -> decltype(select_impl<T>(t));
            template <typename T, typename... Ts>
            static auto select_base_(tuple<Ts...> t) -> decltype(select_base_impl<T>(t));

        public:
            template <typename T, typename Tuple>
            using type = decltype(select_<T>(std::declval<Tuple>()));

            template <typename T, typename Tuple>
            using base_type = decltype(select_base_<T>(std::declval<Tuple>()));
        };

        // is_tuple_impl

        template <typename> struct is_tuple_impl : std::false_type {};

        template <std::size_t... Is, typename... Ts>
        struct is_tuple_impl<tuple_base<std::index_sequence<Is...>, Ts...>> : std::true_type { };

        // tuple_cat

        template <std::size_t... Inner, std::size_t... Outer, typename Tuples>
        constexpr static auto tuple_cat(std::index_sequence<Inner...>, std::index_sequence<Outer...>, Tuples&& ts);

        template <typename... Ts, std::size_t... Is>
        constexpr static auto tuple_cat(std::index_sequence<Is...>, Ts&&... tuples);
    }

    /**
    * An implementation of a tuple.
    * 
    * @remark This implementation differs from the standard on several key points:
    *         1. It is non-recursive (not specified by the standard, but that's how most STL implementations do it)
    *         2. It only accepts being constructed from values of its elements.
    */
    template <typename... Ts>
    class tuple : private detail::tuple_base<std::index_sequence_for<Ts...>, Ts...> {
        // Not exposed; needed by many internal traits to bypass private inheritance
        using base_t = detail::tuple_base<std::index_sequence_for<Ts...>, Ts...>;

        friend class detail::select_base_by_index;
        friend class detail::select_base_by_type;

        template <typename T, typename Tuple> friend auto get(Tuple&& tuple) -> typename detail::select_base_by_type::type<T, Tuple>;
        template <std::size_t I, typename Tuple> friend auto get(Tuple&& tuple) -> typename detail::select_base_by_index::type<I, Tuple>;

        template <typename... Xs, std::size_t... Is>
        friend constexpr auto detail::tuple_cat(std::index_sequence<Is...>, Xs&&... tuples);

    public:
        /**
         * Construct a tuple with the given values. 
         */
        tuple(Ts&&... args) : base_t(std::forward<Ts>(args)...) { };

        template <std::size_t I = sizeof...(Ts), typename = std::enable_if_t<(I > 0)>> tuple() { };

        tuple(tuple const&) = default;
        tuple(tuple&&) noexcept = default;

        tuple& operator = (tuple const&) = default;
        tuple& operator = (tuple&&) = default;

        /**
        * Get type of element of tuple at index I.
        */
        template <std::size_t I>
        using at_offset = typename detail::select_base_by_index::template type<I, base_t>;

        /**
        * Get type of tuple element storage at index I.
        */
        template <std::size_t I>
        using base_at_offset = typename detail::select_base_by_index::template base_type<I, base_t>;

        /**
        * Get element of tuple of type T.
        */
        template <typename T>
        using of_type = typename detail::select_base_by_type::template type<T, base_t>;

        /**
        * Get tuple element storage of type T of tuple.
        */
        template <typename T>
        using base_of_type = typename detail::select_base_by_type::template base_type<T, base_t>;
    };

    /**
    * Metafunction returning the type of the element of the tuple at index I.
    */
    template <std::size_t I, typename Tuple> struct tuple_element {
        using type = typename Tuple::template at_offset<I>;
    };

    template <std::size_t I, typename Tuple>
    using tuple_element_t = typename tuple_element<I, Tuple>::type;

    /**
    * Metafunction returning the size of a tuple.
    */
    template <typename... Ts> struct tuple_size<tuple<Ts...>> { enum { size = sizeof...(Ts) }; };
    template <typename T> constexpr static const std::size_t tuple_size_v = tuple_size<T>::size;

    /**
    * Metafunction returning the index of the given type in the tuple.
    */
    template <typename T, typename Tuple>
    constexpr static const std::size_t tuple_index_v = Tuple::template base_of_type<T>::index;

    /**
    * Function returning the I-th element of a tuple.
    */
    template <std::size_t I, typename Tuple>
    auto get(Tuple&& tuple) -> typename detail::select_base_by_index::type<I, Tuple> {
        return static_cast<
            typename detail::select_base_by_index::base_type<I, std::decay_t<Tuple>>&&
        >(tuple).value_;
        // return static_cast<typename std::decay_t<Tuple>::template base_at_offset<I>&&>(tuple).value_;
    }

    /**
    * Function returning the element of a tuple that is of type T.
    */
    template <typename T, typename Tuple>
    auto get(Tuple&& tuple) -> typename detail::select_base_by_type::type<T, Tuple> {
        return static_cast<
            typename detail::select_base_by_type::base_type<T, std::decay_t<Tuple>>
        >(tuple).value_;
        // return static_cast<typename std::decay_t<Tuple>::template base_of_type<T>&&>(tuple).value_;
    }

    /**
    * Implementation of tuple_cat. Inspired by Stefan T. Lavavej's approach to tuple_cat.
    */
    namespace detail {
        template <typename... Tuples>
        constexpr static auto tuple_cat_index_gen() {
            constexpr const std::size_t elem_count = (tuple_size_v<Tuples> + ...);
            constexpr const std::size_t tuple_count = sizeof...(Tuples);

            constexpr const std::size_t tuple_sizes[] = { tuple_size_v<Tuples>... };

            std::array<std::size_t, elem_count> inner;
            std::array<std::size_t, elem_count> outer;

            std::size_t globalIndex = 0;
            for (std::size_t i = 0; i < tuple_count; ++i) {
                for (std::size_t j = 0; j < tuple_sizes[i]; ++j) {
                    inner[globalIndex] = i;
                    outer[globalIndex] = j;
                    ++globalIndex;
                }
            }

            return std::pair { inner, outer };
        }

        template <std::size_t... Inner, std::size_t... Outer, typename Tuples>
        constexpr auto tuple_cat(std::index_sequence<Inner...>, std::index_sequence<Outer...>, Tuples&& ts) {
            return tuple { get<Outer>(get<Inner>(ts))... };
        }

        template <typename... Ts, std::size_t... Is>
        constexpr auto tuple_cat(std::index_sequence<Is...>, Ts&&... tuples) {
            constexpr const auto index_pairs = tuple_cat_index_gen<Ts...>();
            return tuple_cat(std::index_sequence<index_pairs.first[Is]...> { },
                std::index_sequence<index_pairs.second[Is]...> { },
                tuple<Ts...> { std::forward<Ts>(tuples)... });
        }
    }

    template <typename... Ts>
    auto tuple_cat(Ts&&... tuples) {
        constexpr static const std::size_t elem_count = (tuple_size_v<Ts> + ...);
        return detail::tuple_cat(std::make_index_sequence<elem_count> { }, std::forward<Ts&&>(tuples)...);
    }

    /**
    * Implementation of std::tie.
    */
    template <typename... Ts>
    tuple<Ts&...> tie(Ts&... elements) {
        return tuple<Ts&...> { elements... };
    }

    /**
    * Utility type to discard assignments through tie(...).
    */
    static struct ignore_t {
        template <typename T> ignore_t& operator = (T&&) { return *this; }
    } _;
}

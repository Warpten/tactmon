#pragma once

#include <boost/mp11/algorithm.hpp>

#include <array>
#include <cstdint>
#include <type_traits>
#include <utility>

#ifndef TACTMON_NO_UNIQUE_ADDRESS
# if _WIN32
#  if _MSC_VER >= 1929 
#   define TACTMON_NO_UNIQUE_ADDRESS [[msvc::no_unique_address]]
#  else
#   define TACTMON_NO_UNIQUE_ADDRESS [[no_unique_address]]
#  endif
# else
#  define TACTMON_NO_UNIQUE_ADDRESS [[no_unique_address]]
# endif
#endif // TACTMON_NO_UNIQUE_ADDRESS

namespace utility {
    template <typename...> class tuple;

    namespace detail {
        namespace mp = boost::mp11;

        template <std::size_t Internal, std::size_t Interface, typename T>
        struct tuple_leaf {
            constexpr static const auto class_layout_index = Internal;
            constexpr static const auto interface_index = Interface;

            constexpr tuple_leaf() noexcept (std::is_nothrow_constructible_v<T>) 
                : _value() { }

            tuple_leaf(tuple_leaf const&) = default;
            tuple_leaf(tuple_leaf&&) noexcept = default;
            tuple_leaf& operator = (tuple_leaf const&) = default;
            tuple_leaf& operator = (tuple_leaf&&) noexcept = default;

            template <typename U>
            requires (std::is_constructible_v<T, U>)
            explicit constexpr tuple_leaf(U&& value) noexcept (std::is_nothrow_constructible_v<T, U>)
                : _value(std::forward<U>(value)) { }

            constexpr void swap(tuple_leaf&& other) noexcept(std::is_nothrow_swappable_v<T>) {
                using std::swap;
                swap(_value, other._value);
            }

            TACTMON_NO_UNIQUE_ADDRESS T _value;
        };

        template <std::size_t I, typename T> struct indexed { };

        // Plug alignment_of with std::alignment_of and overload indexed<I, T> specialization
        template <typename T> struct alignment_of {
            constexpr static const std::size_t value = std::alignment_of<T>::value;
        };
        template <std::size_t I, typename T> struct alignment_of<indexed<I, T>> {
            constexpr static const std::size_t value = std::alignment_of<T>::value;
        };

        // Max alignment of all types in a list
        template <typename> struct max_alignment_of;
        template <typename... Ts> struct max_alignment_of<mp::mp_list<Ts...>> {
            constexpr static const std::size_t value = std::max({ alignment_of<Ts>::value... });
        };

        template <typename L, typename R>
        using sort_pred = std::bool_constant<(alignment_of<L>::value >= alignment_of<R>::value)>;

        template <typename T, typename C> using make_indexed = indexed<C::value, T>;

        // given a length L, produces a mp_list_c of said length starting from 0.
        template <typename> struct make_mp_list_c;
        template <std::size_t... Is> struct make_mp_list_c<std::index_sequence<Is...>> {
            using type = mp::mp_list_c<std::size_t, Is...>;
        };
        template <std::size_t L>
        using make_mp_list_c_t = typename make_mp_list_c<std::make_index_sequence<L>>::type;

        // Sorts the input type list and indexes it
        template <typename Ins>
        using indexed_sort = mp::mp_transform<
            make_indexed,
            mp::mp_sort<Ins, sort_pred>,
            make_mp_list_c_t<mp::mp_size<Ins>::value>
        >;

        // Given Is... & Ts..., yields type_list<indexed<Is, Ts>...>
        template <typename, typename> struct indexer;
        template <std::size_t... Is, typename... Ts>
        struct indexer<std::index_sequence<Is...>, mp::mp_list<Ts...>> {
            using type = mp::mp_list<indexed<Is, Ts>...>;
        };

        // given ...Ts, yields type_list<indexed<I (calculated), T>...>
        template <typename... Ts>
        using indexed_types = typename indexer<std::index_sequence_for<Ts...>, mp::mp_list<Ts...>>::type;

        // Pair of indices (storage <-> interface)
        template <std::size_t Out, std::size_t In> struct mapping { };

        template <typename> struct tuple_storage;
        template <std::size_t... Outs, std::size_t... Ins, typename... Ts>
        class tuple_storage<
            mp::mp_list<
                indexed<
                    Outs,
                    indexed<Ins, Ts>
                >...
            >
        > : public tuple_leaf<Outs, Ins, Ts>... {
            template <std::size_t I, std::size_t O, typename T>
            constexpr static auto select_leaf_front(tuple_leaf<O, I, T>) -> tuple_leaf<O, I, T>;

            template <std::size_t O, std::size_t I, typename T>
            constexpr static auto select_leaf_back(tuple_leaf<O, I, T>) -> tuple_leaf<O, I, T>;

            template <std::size_t I>
            using leaf_for_front = decltype(select_leaf_front<I>(std::declval<tuple_storage>()));

            template <std::size_t I>
            using leaf_for_back = decltype(select_leaf_back<I>(std::declval<tuple_storage>()));
        public:
            /**
             * Maps internal and external indices.
             */
            using indices_map = mp::mp_list<mapping<Outs, Ins>...>;

            /**
             * Constructs a new storage if std::is_constructible_v<Ts[I], Us[I]> for all Is.
             *
             * @tparam ...Is An index sequence for each leaf
             * @tparam ...Us Argument types.
             */
            template <std::size_t... Is, typename... Us>
            requires (sizeof...(Is) == sizeof...(Us))
            constexpr explicit tuple_storage(std::index_sequence<Is...>, Us&&... args)
                noexcept((std::is_nothrow_constructible_v<leaf_for_front<Is>, Us> && ...))
                : leaf_for_front<Is>(std::forward<Us>(args))...
            { }

            constexpr void swap(tuple_storage&& other)
                noexcept((std::is_nothrow_swappable_v<Ts> && ...))
            {
                (tuple_leaf<Outs, Ins, Ts>::swap(std::forward<tuple_leaf<Outs, Ins, Ts>>(other)), ...);
            }
        };

        template <typename... Ts>
        using make_tuple_storage = tuple_storage<indexed_sort<indexed_types<Ts...>>>;

        // Resolves tuple_leaf for an I over tuple_storage
        template <std::size_t I>
        struct select_base_impl_i {
            template <std::size_t O, typename T>
            constexpr static auto select_impl_(detail::tuple_leaf<O, I, T> leaf) { return leaf; }

            template <typename... Ts>
            constexpr static auto select_(tuple<Ts...>);

#define GET_IMPL(QUALIFIERS)                                                                \
            template <typename... Ts>                                                       \
            static constexpr auto&& get(tuple<Ts...> QUALIFIERS tpl) noexcept {             \
                using selected_base_type = decltype(select_(std::declval<tuple<Ts...>>())); \
                return static_cast<selected_base_type QUALIFIERS>(tpl)._value;              \
            }

            GET_IMPL(&&)
            GET_IMPL(&)
            GET_IMPL(const &)
            GET_IMPL(const &&)
#undef GET_IMPL
        };

        template <std::size_t I, typename T>
        using select_base_i = decltype(select_base_impl_i<I>::template select_(std::declval<T>()));

        // Resolve tuple_leaf for a T over tuple_storage
        template <typename T>
        struct select_base_impl_t {
            template <std::size_t I, std::size_t O>
            constexpr static auto select_impl_(detail::tuple_leaf<I, O, T> leaf) { return leaf; }

            template <typename... Ts>
            constexpr static auto select_(tuple<Ts...>);

#define GET_IMPL(QUALIFIERS)                                                                \
            template <typename... Ts>                                                       \
            static constexpr auto&& get(tuple<Ts...> QUALIFIERS tpl) noexcept {             \
                using selected_base_type = decltype(select_(std::declval<tuple<Ts...>>())); \
                return static_cast<selected_base_type QUALIFIERS>(tpl)._value;              \
            }

            GET_IMPL(&&)
            GET_IMPL(&)
            GET_IMPL(const &)
            GET_IMPL(const &&)
#undef GET_IMPL
        };

        template <typename T, typename Tuple>
        using select_base_t = decltype(select_base_impl_t<T>::template select_(std::declval<Tuple>()));
    }

    template <typename... Ts> tuple<Ts&&...> forward_as_tuple(Ts&&... args) noexcept;

    template <typename... Ts>
    class tuple final : private detail::make_tuple_storage<Ts...> {
        using storage_t = detail::make_tuple_storage<Ts...>;

        template <std::size_t I> friend struct detail::select_base_impl_i;
        template <typename T> friend struct detail::select_base_impl_t;

    public:
        template <typename... Us>
        explicit constexpr tuple(Us&&... args)
            : storage_t(std::index_sequence_for<Us...> { }, std::forward<Us>(args)...)
        { }

        constexpr void swap(tuple&& other) noexcept((std::is_nothrow_swappable_v<Ts> && ...)) {
            storage_t::swap(std::forward<storage_t>(other));
        }

        template <typename... Us>
        requires ((std::is_copy_assignable_v<Ts> && ...) && (std::is_same_v<Ts, Us> && ...))
        constexpr tuple& operator = (tuple<Us...> const& other)
        {
            static_cast<storage_t&>(*this) = static_cast<const storage_t&>(*other);
            return *this;
        }

#if __cplusplus > 202002L // C++23 only
        template <typename... Us>
        requires ((std::is_copy_assignable_v<const Ts> && ...) && (std::is_same_v<Ts, Us> && ...))
        constexpr const tuple& operator = (tuple<Us...> const& other ) const
        {
            static_cast<const storage_t&>(*this) = static_cast<const storage_t&>(*other);
            return *this;
        }
#endif

        template <typename... Us>
        requires ((std::is_move_assignable_v<Ts> && ...) && (std::is_same_v<Ts, Us> && ...))
        constexpr tuple& operator = (tuple<Us...>&& other) noexcept {
            static_cast<storage_t&>(*this) = static_cast<storage_t&&>(*other);
            return *this;
        }
    };

    template <typename... Ts> tuple<Ts&&...> forward_as_tuple(Ts&&... args) noexcept {
        return tuple<Ts&&...> { std::forward<Ts>(args)... };
    }

    static_assert(sizeof(tuple<>) == 1);

    // ^^^ tuple / tuple_size(_v) vvv

    template <typename> struct tuple_size;
    template <typename... Ts> struct tuple_size<tuple<Ts...>> {
        constexpr static const std::size_t value = sizeof...(Ts);
    };
    template <typename T> struct tuple_size<T&> : tuple_size<T> { };
    template <typename T> struct tuple_size<T const&> : tuple_size<T> { };
    template <typename T> constexpr static const std::size_t tuple_size_v = tuple_size<T>::value;

    // ^^^ tuple_size(_v) / tuple_element(_t) vvv

    template <std::size_t, typename> class tuple_element;
    template <std::size_t I, typename T> class tuple_element<I, T&> : public tuple_element<I, T> { };
    template <std::size_t I, typename T> class tuple_element<I, T&&> : public tuple_element<I, T> { };
    template <std::size_t I, typename T> class tuple_element<I, T const&> : public tuple_element<I, T> { };
    template <std::size_t I, typename... Ts> class tuple_element<I, tuple<Ts...>> {
        template <std::size_t O, typename T>
        constexpr static auto select_(detail::tuple_leaf<O, I, T>) -> T;

    public:
        using type = decltype(select_(std::declval<detail::make_tuple_storage<Ts...>>()));
    };

    template <std::size_t I, typename T>
    using tuple_element_t = typename tuple_element<I, T>::type;

    // ^^^ tuple_element(_t) / get vvv

    template <std::size_t I>
    template <typename... Ts>
    constexpr auto detail::select_base_impl_i<I>::select_(tuple<Ts...> tpl) {
        return select_impl_(tpl);
    }

    template <typename T>
    template <typename... Ts>
    constexpr auto detail::select_base_impl_t<T>::select_(tuple<Ts...> tpl) {
        return select_impl_(tpl);
    }

    template <std::size_t I, typename Tuple>
    constexpr auto&& get(Tuple&& tpl) {
        return detail::select_base_impl_i<I>::get(std::forward<Tuple>(tpl));
    }

    template <typename T, typename Tuple>
    constexpr auto&& get(Tuple&& tpl) {
        return detail::select_base_impl_t<T>::get(std::forward<Tuple>(tpl));
    }

    // ^^^ get / ignore vvv

    namespace detail {
        struct ignore_t final {
            template <typename T>
            constexpr void operator = (T&&) const noexcept { }
        };
    }

    inline constexpr detail::ignore_t ignore;

    // ^^^ ignore / tie vvv

    template <typename... Args>
    constexpr tuple<Args&...> tie(Args&... args) noexcept {
        return tuple { args... };
    }

    // ^^^ tie / tuple_cat vvv

    namespace detail {
        template <typename... Tuples>
        constexpr static auto tuple_cat_index_gen() {
            constexpr const std::size_t elem_count = (tuple_size_v<Tuples> + ... + 0);
            constexpr const std::size_t tuple_count = sizeof...(Tuples);

            constexpr const std::size_t tuple_sizes[] = { tuple_size_v<Tuples>..., 0 };

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

        template <std::size_t... Inner, std::size_t... Outer, typename... Ts>
        constexpr auto tuple_cat_impl(std::index_sequence<Inner...> is, std::index_sequence<Outer...> os, tuple<Ts...>&& ts) {
            return tuple<
                tuple_element_t<
                    Outer,
                    tuple_element_t<
                        Inner,
                        tuple<Ts...>
                    >
                >...
            > { get<Outer>(get<Inner>(ts))... };
        }

        template <typename... Ts, std::size_t... Is>
        constexpr auto tuple_cat_impl(std::index_sequence<Is...>, Ts... tuples) {
            constexpr const auto index_pairs = tuple_cat_index_gen<Ts...>();
            return tuple_cat_impl(std::index_sequence<index_pairs.first[Is]...> { },
                std::index_sequence<index_pairs.second[Is]...> { },
                tuple<Ts...> { std::forward<Ts>(tuples)... });
        }
    }

    template <typename... Ts>
    auto tuple_cat(Ts&&... tuples) {
        constexpr static const std::size_t elem_count = (tuple_size_v<Ts> + ... + 0);
        if constexpr (elem_count == 0)
            return tuple<> { };
        else
            return detail::tuple_cat_impl(std::make_index_sequence<elem_count> { }, std::forward<Ts>(tuples)...);
    }
}

#undef TACTMON_NO_UNIQUE_ADDRESS

#pragma once

#include "backend/db/orm/Concepts.hpp"
#include "backend/db/orm/detail/VariadicRenderable.hpp"
#include "utility/Tuple.hpp"

#include <ostream>
#include <type_traits>
#include <utility>

#include <pqxx/row>

namespace backend::db {
    namespace detail {
        // Specific storage for a projection component
        template <typename COMPONENT>
        struct column_storage : public COMPONENT {
            using value_type = typename COMPONENT::value_type;

            column_storage() { }
            explicit column_storage(value_type&& value) noexcept : value_(std::move(value)) { }
            explicit column_storage(value_type const& value) : value_(value) { }

            value_type value_ { };
        };

        // Propagates indexed access through a tuple for a projection's components.
        template <typename... COLUMNS>
        struct column_tuple {
            column_tuple() : columns_() { }

            template <std::size_t... Is>
            column_tuple(std::index_sequence<Is...>, pqxx::row const& row) : columns_(row[Is].as<typename COLUMNS::value_type>()...)
            { }

            utility::tuple<COLUMNS...> columns_;
        };

        /**
         * Utility type that provides the ability to bind a component to a projection or entity.
         */
        template <typename T> struct BindComponent {
            template <typename ENTITY>
            using ToProjection = T;
        };

        template <utility::Literal NAME, typename T>
        struct BindComponent<Column<NAME, T>> {
            template <typename ENTITY>
            using ToProjection = typename Column<NAME, T>::template Of<ENTITY>;
        };
    }

    /**
     * A projection is a collection of components of varying types.
     *
     * @tparam COLUMNS... Types describing elements of the projection.
     */
    template <typename... COLUMNS>
    struct Projection final {
        using parameter_types = decltype(utility::tuple_cat(
            std::declval<typename COLUMNS::parameter_types>()...
        ));

        Projection() : _columns() { }
        explicit Projection(pqxx::row const& row) : _columns(std::make_index_sequence<sizeof...(COLUMNS)> { }, row) { }

        template <std::size_t P>
        static auto render_to(std::ostream& ss, std::integral_constant<std::size_t, P> p) {
            return detail::VariadicRenderable<", ", COLUMNS...>::render_to(ss, p);
        }

    public:
        /**
        * Provides structured-binding access to this projection.
        */
        template <std::size_t I> requires (I < sizeof...(COLUMNS))
        auto&& get() const {
            return utility::get<I>(_columns.columns_).value_;
        }

        template <std::size_t I> requires (I < sizeof...(COLUMNS))
        auto&& get() {
            return utility::get<I>(_columns.columns_).value_;
        }

        template <typename T>
        auto&& get() const {
            return utility::get<detail::column_storage<T>>(_columns.columns_).value_;
        }

        template <typename T>
        auto&& get() {
            return utility::get<detail::column_storage<T>>(_columns.columns_).value_;
        }

    private:
        detail::column_tuple<detail::column_storage<COLUMNS>...> _columns;
    };

    template <std::size_t I, typename... COMPONENTS>
    auto&& get(Projection<COMPONENTS...>& proj) {
        return proj.template get<I>();
    }

    template <typename T, typename... COMPONENTS>
    auto&& get(Projection<COMPONENTS...>& proj) {
        return proj.template get<T>();
    }

    template <std::size_t I, typename... COMPONENTS>
    auto&& get(Projection<COMPONENTS...> const & proj) {
        return proj.template get<I>();
    }

    template <typename T, typename... COMPONENTS>
    auto&& get(Projection<COMPONENTS...> const& proj) {
        return proj.template get<T>();
    }
}

// Structured bindings requirement
template <typename... COMPONENTS>
struct std::tuple_size<backend::db::Projection<COMPONENTS...>> {
    constexpr static const size_t value = sizeof...(COMPONENTS);
};

template <std::size_t I, typename... COMPONENTS>
struct std::tuple_element<I, backend::db::Projection<COMPONENTS...>> {
    using type = decltype(std::declval<backend::db::Projection<COMPONENTS...>>().template get<I>());
};

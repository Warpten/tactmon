#pragma once

#include "backend/db/orm/Concepts.hpp"
#include "backend/db/orm/Column.hpp"
#include "backend/db/orm/Projection.hpp"

#include <ostream>
#include <type_traits>
#include <utility>

namespace backend::db {
    /**
     * Describes a database table.
     *
     * @tparam NAME          The name of the table.
     * @tparam SCHEMA        The schema holding the table.
     * @tparam COMPONENTS... Specializations of Column that this entity holds.
     */
    template <utility::Literal NAME, utility::Literal SCHEMA, typename... COMPONENTS>
    struct Entity final {
        using parameter_types = decltype(utility::tuple_cat(
            std::declval<typename COMPONENTS::parameter_types>()...
        ));

        /**
        * Interprets this entity as a bound projection to itself.
        */
        using projection_type = Projection<typename COMPONENTS::template Of<Entity<NAME, SCHEMA, COMPONENTS...>>...>;

        /**
        * The primary key associated with this entity.
        */
        using primary_key_type = utility::tuple_element_t<0, utility::tuple<COMPONENTS...>>;

        constexpr static const auto Name = NAME;

    public:
        Entity() : _proj() { }
        explicit Entity(pqxx::row const& row) : _proj(row) { }
        Entity(projection_type const& proj) : _proj(proj) { }

        template <std::size_t PARAMETER>
        constexpr static auto render_to(std::string prev, std::integral_constant<std::size_t, PARAMETER> p) {
            return std::make_pair(prev + SCHEMA.Value + '.' + NAME.Value, p);
        }

    public:
        /**
         * Accesses a column of this entity.
         *
         * @tparam I Index of the column to read.
         */
        template <std::size_t I> requires (I < sizeof...(COMPONENTS))
        auto&& get() {
            return _proj.template get<I>();
        }

        /**
         * Accesses a column of this entity.
         *
         * @tparam I Index of the column to read.
         */
        template <std::size_t I> requires (I < sizeof...(COMPONENTS))
        auto&& get() const {
            return _proj.template get<I>();
        }

        /**
        * Accesses a column of this entity.
        *
        * @tparam T Type of the column to read.
        */
        template <typename T>
        auto&& get() {
            using bound_column_type = typename T::template BindToProjection<Entity<NAME, SCHEMA, COMPONENTS...>>;

            return _proj.template get<bound_column_type>();
        }

        template <typename T>
        auto&& get() const {
            using bound_column_type = typename T::template BindToProjection<Entity<NAME, SCHEMA, COMPONENTS...>>;

            return _proj.template get<bound_column_type>();
        }

    private:
        projection_type _proj;
    };

    template <std::size_t I, utility::Literal NAME, utility::Literal SCHEMA, typename... COMPONENTS>
    auto&& get(Entity<NAME, SCHEMA, COMPONENTS...>& proj) {
        return proj.template get<I>();
    }

    template <typename T, utility::Literal NAME, utility::Literal SCHEMA, typename... COMPONENTS>
    auto&& get(Entity<NAME, SCHEMA, COMPONENTS...> const& proj) {
        return proj.template get<T>();
    }

    template <typename T, utility::Literal NAME, utility::Literal SCHEMA, typename... COMPONENTS>
    auto&& get(Entity<NAME, SCHEMA, COMPONENTS...>& proj) {
        return proj.template get<T>();
    }

    template <std::size_t I, utility::Literal NAME, utility::Literal SCHEMA, typename... COMPONENTS>
    auto&& get(Entity<NAME, SCHEMA, COMPONENTS...> const& proj) {
        return proj.template get<I>();
    }
}

// Structured bindings requirement
template <utility::Literal NAME, utility::Literal SCHEMA, typename... COMPONENTS>
struct std::tuple_size<backend::db::Entity<NAME, SCHEMA, COMPONENTS...>> {
    constexpr static const std::size_t value = sizeof...(COMPONENTS);
};

template <std::size_t I, utility::Literal NAME, utility::Literal SCHEMA, typename... COMPONENTS>
struct std::tuple_element<I, backend::db::Entity<NAME, SCHEMA, COMPONENTS...>> {
    using type = decltype(std::declval<backend::db::Entity<NAME, SCHEMA, COMPONENTS...>>().template get<I>());
};

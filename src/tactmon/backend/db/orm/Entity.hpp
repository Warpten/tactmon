#pragma once

#include "backend/db/orm/concepts/Concepts.hpp"
#include "backend/db/orm/Column.hpp"
#include "backend/db/orm/Projection.hpp"

#include <ostream>
#include <type_traits>
#include <utility>

namespace backend::db::orm {
    /*namespace detail {
        template <typename T>
        struct BindComponent {
            template <typename ENTITY>
            using ToProjection = T;
        };

        template <utility::Literal NAME, typename T>
        struct BindComponent<Column<NAME, T>>
        {
            template <typename ENTITY>
            using ToProjection = typename Column<NAME, T>::template Of<ENTITY>;
        };
    }*/

    /**
     * Describes a database table.
     *
     * @tparam NAME          The name of the table.
     * @tparam SCHEMA        The schema holding the table.
     * @tparam COMPONENTS... Specializations of Column that this entity holds.
     */
    template <utility::Literal NAME, utility::Literal SCHEMA, concepts::StreamRenderable... COMPONENTS>
    struct Entity final {
        template <size_t PARAMETER>
        static auto render_to(std::ostream& stream, std::integral_constant<size_t, PARAMETER> p) {
            stream << SCHEMA.Value << '.' << NAME.Value;
            return p;
        }

        /**
        * Interprets this entity as a bound projection to itself.
        */
        using projection_type = Projection<typename COMPONENTS::template Of<Entity<NAME, SCHEMA, COMPONENTS...>>...>;

        /**
        * The primary key associated with this entity.
        */
        using primary_key_type = utility::tuple_element_t<0, utility::tuple<COMPONENTS...>>;

    public:
        /**
         * Accesses a column of this entity.
         *
         * @tparam I Index of the column to read.
         */
        template <size_t I> requires (I < sizeof...(COMPONENTS))
        auto&& get() {
            return _proj.template get<I>();
        }

        /**
         * Accesses a column of this entity.
         *
         * @tparam I Index of the column to read.
         */
        template <size_t I> requires (I < sizeof...(COMPONENTS))
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
            using bound_column_type = typename T::template Bind<Entity<NAME, SCHEMA, COMPONENTS...>>;

            return _proj.template get<bound_column_type>();
        }

        template <typename T>
        auto&& get() const {
            using bound_column_type = typename T::template Bind<Entity<NAME, SCHEMA, COMPONENTS...>>;

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
struct std::tuple_size<backend::db::orm::Entity<NAME, SCHEMA, COMPONENTS...>> {
    constexpr static const size_t value = sizeof...(COMPONENTS);
};

template <size_t I, utility::Literal NAME, utility::Literal SCHEMA, typename... COMPONENTS>
struct std::tuple_element<I, backend::db::orm::Entity<NAME, SCHEMA, COMPONENTS...>> {
    using type = decltype(std::declval<backend::db::orm::Entity<NAME, SCHEMA, COMPONENTS...>>().template get<I>());
};

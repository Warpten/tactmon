#pragma once

#include <ext/Tuple.hpp>
#include <ext/Literal.hpp>

#include <pqxx/row>

namespace backend::db {
    struct Ignore { };

    namespace detail {
        template <typename COMPONENT>
        struct column_storage : COMPONENT {
            using value_type = typename COMPONENT::value_type;

            column_storage() : _value() { }
            column_storage(value_type const& value ) : _value(value) { }
            column_storage(value_type&& value ) : _value(std::move(value)) { }

            value_type _value;
        };

        template <typename... STORAGES>
        struct column_tuple {

            column_tuple() : _tuple() { }

            template <size_t... Is>
            column_tuple(std::index_sequence<Is...>, pqxx::row& row) : _tuple(row[Is].as<typename STORAGES::value_type>()...) 
            { }

            ext::tuple<STORAGES...> _tuple;
        };
    }

    template <ext::Literal NAME, typename TYPE>
    struct Column {
        using value_type = TYPE;
    };

    template <ext::Literal ALIAS, typename COMPONENT>
    struct Alias {
        using value_type = typename COMPONENT::value_type;
    };

    template <typename... COMPONENTS>
    struct Projection {
        explicit Projection(pqxx::row& row) : _store(std::make_index_sequence<sizeof...(COMPONENTS)>{}, row) { }

        template <typename T, typename P>
        friend auto get(P&&) -> typename T::value_type;

        detail::column_tuple<detail::column_storage<COMPONENTS>...> _store;
    };

    /**
    * Returns the value of a given column in a projection.
    */
    template <typename COLUMN, typename PROJECTION>
    auto get(PROJECTION&& projection) -> typename COLUMN::value_type {
        return ext::get<detail::column_storage<COLUMN>>(projection._store._tuple)._value;
    }

    template <ext::Literal NAME, ext::Literal SCHEMA, typename... COMPONENTS>
    struct Entity : Projection<COMPONENTS...> {
        explicit Entity() { }

        constexpr static const auto Name = NAME;

        using as_projection = Projection<COMPONENTS...>;
    };

    template <ext::Literal FORMAT, typename COMPONENT>
    struct Criteria {
        constexpr static const size_t criteria_count = 1;
    };

    template <typename COMPONENT> using Equals = Criteria<"{} = ${}", COMPONENT>;
    template <typename COMPONENT> using GreaterThan = Criteria<"{} > ${}", COMPONENT>;
    template <typename COMPONENT> using LessThan = Criteria<"{} < ${}", COMPONENT>;
    template <typename COMPONENT> using GreaterOrEqual = Criteria<"{} >= ${}", COMPONENT>;
    template <typename COMPONENT> using LessOrEqual = Criteria<"{} <= ${}", COMPONENT>;

    template <ext::Literal TOKEN, typename... CRITERIAS>
    struct ManyCriteria {
        constexpr static const size_t criteria_count = (CRITERIAS::criteria_count + ... + 0);
    };

    template <typename... CRITERIAS> using And = ManyCriteria<" AND ", CRITERIAS...>;
    template <typename... CRITERIAS> using Or = ManyCriteria<" OR ", CRITERIAS...>;

    template <ext::Literal FORMAT, typename T, typename... COMPONENTS>
    struct Function {
        static_assert(sizeof...(COMPONENTS) > 0, "SQL functions neeed at least one argument (for now). Should you need to invoke a parameterless function, provide db::Ignore.");

        using value_type = T;
    };

    template <typename... COMPONENTS> using Count = Function<"COUNT", uint32_t, COMPONENTS...>;

    template <typename LEFT, typename RIGHT> struct Over {
        using value_type = typename LEFT::value_type;
    };

    template <typename COMPONENT> struct PartitionBy { };

    template <bool ASCENDING, typename... COMPONENT> struct OrderByClause { };
    template <typename... ORDERS> struct OrderBy { };

    template <size_t COUNT, size_t OFFSET> struct Limit { };

    template <typename COMPONENT> struct From { 
        using entity_type = COMPONENT;
    };
    template <typename COMPONENT> struct Where { };
}

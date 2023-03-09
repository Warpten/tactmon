#pragma once

#include "utility/Tuple.hpp"
#include "utility/Literal.hpp"

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

            utility::tuple<STORAGES...> _tuple;
        };
    }

    /**
     * Represents a SQL column.
     * 
     * @typeparam NAME The name of the column.
     * @typeparam TYPE The type of the column.
     */
    template <utility::Literal NAME, typename TYPE>
    struct Column {
        using value_type = TYPE;
    };

    /**
     * Represents a SQL alias. An alias associates another element of the DSL with a name.
     * 
     * @typeparam ALIAS The name of this alias.
     * @typeparam COMPONENT The DSL element being aliased.
     */
    template <utility::Literal ALIAS, typename COMPONENT>
    struct Alias {
        using value_type = typename COMPONENT::value_type;
    };

    /**
     * Represents a SQL projection. A SQL projection is a collection of SQL columns or function returns.
     */
    template <typename... COMPONENTS>
    struct Projection {
        explicit Projection() : _store() { }

        explicit Projection(pqxx::row& row) : _store(std::make_index_sequence<sizeof...(COMPONENTS)>{}, row) { }

        template <typename T, typename P>
        friend auto get(P&&) -> typename T::value_type;

    private:
        detail::column_tuple<detail::column_storage<COMPONENTS>...> _store;
    };

    /**
     * Returns the value of a given column in a projection.
     * 
     * @typeparam COLUMN     The type of the column to extract out of the entity or projection supplied.
     *                       This **has** to be a specialization of @pre Column.
     * @param[in] projection The entity or projection from which a column is to be extracted.
     */
    template <typename COLUMN, typename PROJECTION>
    auto get(PROJECTION&& projection) -> typename COLUMN::value_type {
        return utility::get<detail::column_storage<COLUMN>>(projection._store._tuple)._value;
    }

    /**
     * Represents a SQL entity. A SQL entity encapsulates all of a table's columns.
     * 
     * @typeparam NAME          The name of the table described by this entity.
     * @typeparam SCHEMA        The schema containing the table associated with this entity.
     * @typeparam COMPONENTS... Column components.
     */
    template <utility::Literal NAME, utility::Literal SCHEMA, typename... COMPONENTS>
    struct Entity : Projection<COMPONENTS...> {
        using as_projection = Projection<COMPONENTS...>;

        explicit Entity() : as_projection() { }

        explicit Entity(pqxx::row& row) : as_projection(row) { }

        constexpr static const auto Name = NAME;
    };

    /**
     * Represents a simple binary SQL criteria.
     * 
     * @typeparam FORMAT A std::format-compatible format string for which the first argument 
     *                   is the component being compared, and the second one a named parameter index.
     * @typeparam COMPONENT The component for which the criteria happens.
     */
    template <utility::Literal FORMAT, typename COMPONENT>
    struct Criteria {
        constexpr static const size_t criteria_count = 1;
    };

    template <typename COMPONENT> using Equals = Criteria<"{} = ${}", COMPONENT>;
    template <typename COMPONENT> using GreaterThan = Criteria<"{} > ${}", COMPONENT>;
    template <typename COMPONENT> using LessThan = Criteria<"{} < ${}", COMPONENT>;
    template <typename COMPONENT> using GreaterOrEqual = Criteria<"{} >= ${}", COMPONENT>;
    template <typename COMPONENT> using LessOrEqual = Criteria<"{} <= ${}", COMPONENT>;

    /**
     * An aggregate of many criterias.
     * 
     * @typeparam TOKEN The aggregation token.
     */
    template <utility::Literal TOKEN, typename... CRITERIAS>
    struct ManyCriteria {
        constexpr static const size_t criteria_count = (CRITERIAS::criteria_count + ... + 0);
    };

    template <typename... CRITERIAS> using And = ManyCriteria<" AND ", CRITERIAS...>;
    template <typename... CRITERIAS> using Or = ManyCriteria<" OR ", CRITERIAS...>;

    /**
     * Represents a call to an SQL function.
     * 
     * @typeparam FORMAT        The name of the function.
     * @typeparam T             The type of this function's return value.
     * @typeparam COMPONENTS... Components corresponding to arguments to the function.
     */
    template <utility::Literal FORMAT, typename T, typename... COMPONENTS>
    struct Function {
        static_assert(sizeof...(COMPONENTS) > 0, "SQL functions neeed at least one argument (for now). Should you need to invoke a parameterless function, provide db::Ignore.");

        using value_type = T;
    };
    
    /**
     * Represents the SQL COUNT function.
     * 
     * @typeparam COMPONENT The expression that is being counted.
     */
    template <typename COMPONENT> using Count = Function<"COUNT", uint32_t, COMPONENT>;

    /**
     * Represents the OVER windowing function.
     * 
     * @typeparam LEFT An expression that is the subject of the windowing function.
     * @typeparam RIGHT An expression describing the window function.
     */
    template <typename LEFT, typename RIGHT> struct Over {
        using value_type = typename LEFT::value_type;
    };

    /**
     * Represents the PARTITION BY window function.
     * 
     * @typeparam COMPONENT A component that will be used for partitioning rows.
     */
    template <typename COMPONENT> struct PartitionBy { };

    template <typename ENTITY> struct Returning { };

    /**
     * Represents a compound result set ordering clause.
     * 
     * @typeparam COMPONENT... A sequence of @pre OrderBy components.
     */
    template <bool ASCENDING, typename... COMPONENT> struct OrderByClause { };
    template <typename... ORDERS> struct OrderBy { };

    /**
     * Represents a fixed result set windowing range.
     * 
     * @typeparam COUNT  The maximum amount of rows that will be returned.
     * @typeparam OFFSET The offset of the first row that will be returned across all rows that matched.
     */
    template <size_t COUNT, size_t OFFSET> struct Limit { };

    template <typename COMPONENT> struct From { 
        using entity_type = COMPONENT;
    };
    template <typename COMPONENT> struct Where { };
}

#pragma once

#include "backend/db/DSL.hpp"

#include <ext/Literal.hpp>

#include <pqxx/transaction>

#include <cstdint>
#include <format>
#include <sstream>
#include <string>

namespace backend::db::select {
    namespace detail {
        template <typename... Ts> struct type_list { 
            static_assert(sizeof...(Ts) > 0, "yikes!");
        };
    }

    template <typename PROJECTION, typename FROM, typename CRITERIA = db::Ignore, typename ORDER = db::Ignore, typename LIMIT = db::Ignore>
    struct Query {
        using transaction_type = pqxx::transaction<pqxx::read_committed, pqxx::write_policy::read_only>;
        using projection_type = PROJECTION;
        using entity_type = typename FROM::entity_type;

        // template <ext::Literal ALIAS, typename ADJ>
        // struct With { 
        // };

        static std::string Render();
    };

    namespace renderer {
        template <typename T> struct Proxy { /* exists just to avoid empty ctors on type for which it doesn't make sense. */ };

        template <ext::Literal SEPARATOR, size_t I, typename COMPONENT, typename... COMPONENTS, size_t PARAMETER>
        static auto RenderVaradic(std::ostream& strm, ext::Constant<PARAMETER> paramOffset, ext::Constant<I>, detail::type_list<COMPONENT, COMPONENTS...>);

        // Forward-declare all implementations.
        template <size_t PARAMETER>
        static auto Render(std::ostream& strm, Proxy<Ignore>, ext::Constant<PARAMETER> p);
        template <size_t PARAMETER, ext::Literal NAME, typename TYPE>
        static auto Render(std::ostream& strm, Proxy<Column<NAME, TYPE>>, ext::Constant<PARAMETER> p);
        template <size_t PARAMETER, ext::Literal ALIAS, typename COMPONENT>
        static auto Render(std::ostream& strm, Proxy<Alias<ALIAS, COMPONENT>>, ext::Constant<PARAMETER> p);
        template <size_t PARAMETER, ext::Literal NAME, ext::Literal SCHEMA, typename... COMPONENTS>
        static auto Render(std::ostream& strm, Proxy<Entity<NAME, SCHEMA, COMPONENTS...>>, ext::Constant<PARAMETER> p);
        template <size_t PARAMETER, typename LEFT, typename RIGHT>
        static auto Render(std::ostream& strm, Proxy<Over<LEFT, RIGHT>>, ext::Constant<PARAMETER> p);
        template <size_t PARAMETER, typename... COMPONENTS>
        static auto Render(std::ostream& strm, Proxy<Projection<COMPONENTS...>>, ext::Constant<PARAMETER> p);
        template <size_t PARAMETER, ext::Literal FORMAT, typename COMPONENT>
        static auto Render(std::ostream& strm, Proxy<Criteria<FORMAT, COMPONENT>>, ext::Constant<PARAMETER> p);
        template <size_t PARAMETER, ext::Literal TOKEN, typename... COMPONENTS>
        static auto Render(std::ostream& strm, Proxy<ManyCriteria<TOKEN, COMPONENTS...>>, ext::Constant<PARAMETER> p);
        template <size_t PARAMETER, ext::Literal NAME, typename TYPE, typename... COMPONENTS>
        static auto Render(std::ostream& strm, Proxy<Function<NAME, TYPE, COMPONENTS...>>, ext::Constant<PARAMETER> p);
        template <size_t PARAMETER, bool ASCENDING, typename COMPONENT>
        static auto Render(std::ostream& strm, Proxy<OrderByClause<ASCENDING, COMPONENT>> clause, ext::Constant<PARAMETER> p);
        template <size_t PARAMETER, typename... COMPONENTS>
        static auto Render(std::ostream& strm, Proxy<OrderBy<COMPONENTS...>>, ext::Constant<PARAMETER> p);
        template <size_t PARAMETER, typename PROJECTION, typename ENTITY, typename CRITERIA, typename ORDER, typename LIMIT>
        static auto Render(std::ostream& strm, Proxy<Query<PROJECTION, ENTITY, CRITERIA, ORDER, LIMIT>>, ext::Constant<PARAMETER> p);
        template <size_t PARAMETER, typename CRITERIA>
        static auto Render(std::ostream& strm, Proxy<Where<CRITERIA>>, ext::Constant<PARAMETER> p);
        template <size_t PARAMETER, size_t COUNT, size_t OFFSET>
        static auto Render(std::ostream& strm, Proxy<Limit<COUNT, OFFSET>>, ext::Constant<PARAMETER> p);
        template <size_t PARAMETER, typename COMPONENT>
        static auto Render(std::ostream& strm, Proxy<From<COMPONENT>>, ext::Constant<PARAMETER> p);
        template <size_t PARAMETER, typename COMPONENT>
        static auto Render(std::ostream& strm, Proxy<PartitionBy<COMPONENT>>, ext::Constant<PARAMETER> p);

        // And now implement them all
        template <ext::Literal SEPARATOR, size_t I, typename COMPONENT, typename... COMPONENTS, size_t PARAMETER>
        static auto RenderVaradic(std::ostream& strm, ext::Constant<PARAMETER> paramOffset, ext::Constant<I>, detail::type_list<COMPONENT, COMPONENTS...>) {
            if constexpr (I > 0)
                strm << SEPARATOR.Value;

            auto intermediary = Render(strm, Proxy<COMPONENT> { }, paramOffset);
            if constexpr (sizeof...(COMPONENTS) > 0)
                return RenderVaradic<SEPARATOR>(strm, intermediary, ext::Constant<I + 1> { }, detail::type_list<COMPONENTS...> { });
            else
                return intermediary;
        }

        template <size_t PARAMETER>
        static auto Render(std::ostream& strm, Proxy<Ignore>, ext::Constant<PARAMETER> p) {
            return p;
        }

        template <size_t PARAMETER, ext::Literal NAME, typename TYPE>
        static auto Render(std::ostream& strm, Proxy<Column<NAME, TYPE>>, ext::Constant<PARAMETER> parameterOffset) {
            strm << NAME.ToString();
            return parameterOffset;
        }

        template <size_t PARAMETER, ext::Literal ALIAS, typename COMPONENT>
        static auto Render(std::ostream& strm, Proxy<Alias<ALIAS, COMPONENT>>, ext::Constant<PARAMETER> parameterOffset) {
            auto nextParameterOffset = Render(strm, Proxy<COMPONENT> { }, parameterOffset);
            strm << " AS " << ALIAS.ToString();
            return nextParameterOffset;
        }

        template <size_t PARAMETER, ext::Literal NAME, ext::Literal SCHEMA, typename... COMPONENTS>
        static auto Render(std::ostream& strm, Proxy<Entity<NAME, SCHEMA, COMPONENTS...>>, ext::Constant<PARAMETER> parameterOffset) {
            strm << SCHEMA.ToString() << '.' << NAME.ToString();
            return parameterOffset;
        }

        template <size_t PARAMETER, typename LEFT, typename RIGHT>
        static auto Render(std::ostream& strm, Proxy<Over<LEFT, RIGHT>>, ext::Constant<PARAMETER> parameterOffset) {
            auto nextParameterOffset = Render(strm, Proxy<LEFT> { }, parameterOffset);
            strm << " OVER (";
            auto rightOfs = Render(strm, Proxy<RIGHT> { }, nextParameterOffset);
            strm << ')';
            return rightOfs;
        }

        template <size_t PARAMETER, typename... COMPONENTS>
        static auto Render(std::ostream& strm, Proxy<Projection<COMPONENTS...>>, ext::Constant<PARAMETER> parameterOffset) {
            strm << "SELECT ";
            return RenderVaradic<", ">(strm, parameterOffset, ext::Constant<0> { }, detail::type_list<COMPONENTS...> { });
        }

        template <size_t PARAMETER, ext::Literal FORMAT, typename COMPONENT>
        static auto Render(std::ostream& strm, Proxy<Criteria<FORMAT, COMPONENT>>, ext::Constant<PARAMETER> parameterOffset) {
            std::stringstream temp;
            auto nextParameterOffset = Render(temp, Proxy<COMPONENT> { }, parameterOffset);
            strm << std::format(FORMAT.Value, temp.str(), PARAMETER);
            return ext::Constant<nextParameterOffset.value + 1> { };
        }

        template <size_t PARAMETER, ext::Literal TOKEN, typename... COMPONENTS>
        static auto Render(std::ostream& strm, Proxy<ManyCriteria<TOKEN, COMPONENTS...>>, ext::Constant<PARAMETER> parameterOffset) {
            strm << '(';
            auto result = RenderVaradic<TOKEN>(strm, parameterOffset, ext::Constant<0> { }, detail::type_list<COMPONENTS...> { });
            strm << ')';
            return result;
        }

        template <size_t PARAMETER, ext::Literal NAME, typename TYPE, typename... COMPONENTS>
        static auto Render(std::ostream& strm, Proxy<Function<NAME, TYPE, COMPONENTS...>>, ext::Constant<PARAMETER> parameterOffset) {
            strm << NAME.ToString() << '(';
            auto result = RenderVaradic<", ">(strm, parameterOffset, ext::Constant<0> { }, detail::type_list<COMPONENTS...> { });
            strm << ')';
            return result;
        }

        template <size_t PARAMETER, typename COMPONENT>
        static auto Render(std::ostream& strm, Proxy<PartitionBy<COMPONENT>>, ext::Constant<PARAMETER> parameterOffset) {
            strm << "PARTITION BY ";
            return Render(strm, Proxy<COMPONENT> { }, parameterOffset);
        }

        template <size_t PARAMETER, bool ASCENDING, typename COMPONENT>
        static auto Render(std::ostream& strm, Proxy<OrderByClause<ASCENDING, COMPONENT>> clause, ext::Constant<PARAMETER> parameterOffset) {
            auto nextParameterOffset = Render(strm, Proxy<COMPONENT> { }, parameterOffset);
            strm << (ASCENDING ? " ASC" : " DESC");
            return nextParameterOffset;
        }

        template <size_t PARAMETER, typename... COMPONENTS>
        static auto Render(std::ostream& strm, Proxy<OrderBy<COMPONENTS...>>, ext::Constant<PARAMETER> parameterOffset) {
            strm << "ORDER BY ";
            return RenderVaradic<", ">(strm, parameterOffset, ext::Constant<0> { }, detail::type_list<COMPONENTS...> { });
        }

        template <size_t PARAMETER, typename PROJECTION, typename FROM, typename CRITERIA, typename ORDER, typename LIMIT>
        static auto Render(std::ostream& strm, Proxy<Query<PROJECTION, FROM, CRITERIA, ORDER, LIMIT>>, ext::Constant<PARAMETER> p) {
            auto projectionOffset = Render(strm, Proxy<PROJECTION> { }, p);
            strm << ' ';
            auto entityOffset = Render(strm, Proxy<FROM> { }, projectionOffset);
            strm << ' ';
            auto criteriaOffset = Render(strm, Proxy<CRITERIA> { }, entityOffset);
            strm << ' ';
            auto orderOffset = Render(strm, Proxy<ORDER> { }, entityOffset);
            strm << ' ';
            auto limitOffset = Render(strm, Proxy<LIMIT> { }, orderOffset);
            return limitOffset;
        }

        template <size_t PARAMETER, typename CRITERIA>
        static auto Render(std::ostream& strm, Proxy<Where<CRITERIA>>, ext::Constant<PARAMETER> p) {
            strm << "WHERE ";
            return Render(strm, Proxy<CRITERIA> { }, p);
        }

        // template <size_t PARAMETER, typename PROJECTION, typename ENTITY, typename CRITERIA, ext::Literal ALIAS, typename ADJ>
        // static auto Render(std::ostream& strm, Query<PROJECTION, ENTITY, CRITERIA>::template With<ALIAS, ADJ>, ext::Constant<PARAMETER> p) {
        //     strm << "WITH " << ALIAS.Value << " AS (";
        //     auto projOffset = Render(strm, ADJ { }, p);
        //     strm << " )";
        //     auto queryOffset = Render(strm, Query<PROJECTION, ENTITY, CRITERIA> { }, projOffset);
        //     
        //     return queryOffset;
        // }

        template <size_t PARAMETER, size_t COUNT, size_t OFFSET>
        static auto Render(std::ostream& strm, Proxy<Limit<COUNT, OFFSET>>, ext::Constant<PARAMETER> p) {
            strm << "LIMIT " << COUNT << " OFFSET " << OFFSET;
            return p;
        }

        template <size_t PARAMETER, typename COMPONENT>
        static auto Render(std::ostream& strm, Proxy<From<COMPONENT>>, ext::Constant<PARAMETER> p) {
            strm << "FROM ";
            return Render(strm, Proxy<COMPONENT> { }, p);
        }
    }

    template <typename PROJECTION, typename ENTITY, typename CRITERIA, typename ORDER, typename LIMIT>
    std::string Query<PROJECTION, ENTITY, CRITERIA, ORDER, LIMIT>::Render() {
        std::stringstream ss;
        renderer::Render(ss, renderer::Proxy<Query<PROJECTION, ENTITY, CRITERIA, ORDER, LIMIT>> { }, ext::Constant<1> { });
        return ss.str();
    }

    // template <typename PROJECTION, typename ENTITY, typename CRITERIA, ext::Literal ALIAS, typename ADJ>
    // std::string Query<PROJECTION, ENTITY, CRITERIA>::With<ALIAS, ADJ>::Render() {
    //     std::stringstream ss;
    //     renderer::Render(ss, Query<PROJECTION, ENTITY, CRITERIA>::With<ALIAS, ADJ> { }, ext::Constant<1> { });
    //     return ss.str();
    // }
}

#pragma once

#include "backend/db/DSL.hpp"

#include "utility/Tuple.hpp"
#include "utility/Literal.hpp"

#include <pqxx/transaction>

#include <cstdint>
#include <sstream>
#include <string>

#include <fmt/format.h>

namespace backend::db {
    namespace detail {
        template <typename... Ts> struct type_list { 
            static_assert(sizeof...(Ts) > 0, "yikes!");
        };
    }

    namespace select {
        template <typename PROJECTION, typename FROM, typename CRITERIA = db::Ignore, typename ORDER = db::Ignore, typename LIMIT = db::Ignore>
        struct Query {
            using transaction_type = pqxx::transaction<pqxx::read_committed, pqxx::write_policy::read_only>;
            using projection_type = PROJECTION;
            using entity_type = typename FROM::entity_type;

            static std::string Render();
        };
    }

    namespace insert {
        template <typename COMPONENT>
        struct Value {

        };
        
        template <typename ENTITY, typename... COMPONENTS>
        struct Query {
            using transaction_type = pqxx::work;
            using projection_type = ENTITY;
            using entity_type = ENTITY;

            static std::string Render();
        };
    }

    namespace update {
        template <typename COMPONENT> struct Set { };

        template <typename ENTITY, typename CRITERIA, typename... COMPONENTS>
        struct Query {
            using transaction_type = pqxx::work;
            using projection_type = ENTITY;
            using entity_type = ENTITY;

            static std::string Render();
        };
    }

    namespace del {
        template <typename ENTITY, typename CRITERIA>
        struct Query {
            using transaction_type = pqxx::work;
            using projection_type = ENTITY;
            using entity_type = ENTITY;

            static std::string Render();
        };
    }

    namespace renderer {
        template <typename T> struct Proxy { /* exists just to avoid empty ctors on type for which it doesn't make sense. */ };

        template <utility::Literal SEPARATOR, size_t I, typename COMPONENT, typename... COMPONENTS, size_t PARAMETER>
        static auto RenderVaradic(std::ostream& strm, utility::Constant<PARAMETER> paramOffset, utility::Constant<I>, detail::type_list<COMPONENT, COMPONENTS...>);

        // Forward-declare all implementations.
        template <size_t PARAMETER>
        static auto Render(std::ostream& strm, Proxy<Ignore>, utility::Constant<PARAMETER> p);
        template <size_t PARAMETER, utility::Literal NAME, typename TYPE>
        static auto Render(std::ostream& strm, Proxy<Column<NAME, TYPE>>, utility::Constant<PARAMETER> p);
        template <size_t PARAMETER, utility::Literal ALIAS, typename COMPONENT>
        static auto Render(std::ostream& strm, Proxy<Alias<ALIAS, COMPONENT>>, utility::Constant<PARAMETER> p);
        template <size_t PARAMETER, utility::Literal NAME, utility::Literal SCHEMA, typename... COMPONENTS>
        static auto Render(std::ostream& strm, Proxy<Entity<NAME, SCHEMA, COMPONENTS...>>, utility::Constant<PARAMETER> p);
        template <size_t PARAMETER, typename LEFT, typename RIGHT>
        static auto Render(std::ostream& strm, Proxy<Over<LEFT, RIGHT>>, utility::Constant<PARAMETER> p);
        template <size_t PARAMETER, typename... COMPONENTS>
        static auto Render(std::ostream& strm, Proxy<Projection<COMPONENTS...>>, utility::Constant<PARAMETER> p);
        template <size_t PARAMETER, utility::Literal FORMAT, typename COMPONENT>
        static auto Render(std::ostream& strm, Proxy<Criteria<FORMAT, COMPONENT>>, utility::Constant<PARAMETER> p);
        template <size_t PARAMETER, utility::Literal TOKEN, typename... COMPONENTS>
        static auto Render(std::ostream& strm, Proxy<ManyCriteria<TOKEN, COMPONENTS...>>, utility::Constant<PARAMETER> p);
        template <size_t PARAMETER, utility::Literal NAME, typename TYPE, typename... COMPONENTS>
        static auto Render(std::ostream& strm, Proxy<Function<NAME, TYPE, COMPONENTS...>>, utility::Constant<PARAMETER> p);
        template <size_t PARAMETER, bool ASCENDING, typename COMPONENT>
        static auto Render(std::ostream& strm, Proxy<OrderByClause<ASCENDING, COMPONENT>> clause, utility::Constant<PARAMETER> p);
        template <size_t PARAMETER, typename... COMPONENTS>
        static auto Render(std::ostream& strm, Proxy<OrderBy<COMPONENTS...>>, utility::Constant<PARAMETER> p);
        template <size_t PARAMETER, typename CRITERIA>
        static auto Render(std::ostream& strm, Proxy<Where<CRITERIA>>, utility::Constant<PARAMETER> p);
        template <size_t PARAMETER, size_t COUNT, size_t OFFSET>
        static auto Render(std::ostream& strm, Proxy<Limit<COUNT, OFFSET>>, utility::Constant<PARAMETER> p);
        template <size_t PARAMETER, typename COMPONENT>
        static auto Render(std::ostream& strm, Proxy<From<COMPONENT>>, utility::Constant<PARAMETER> p);
        template <size_t PARAMETER, typename COMPONENT>
        static auto Render(std::ostream& strm, Proxy<PartitionBy<COMPONENT>>, utility::Constant<PARAMETER> p);
        template <size_t PARAMETER, typename COMPONENT>
        static auto Render(std::ostream& strm, Proxy<insert::Value<COMPONENT>>, utility::Constant<PARAMETER> p);

        template <size_t PARAMETER, typename PROJECTION, typename ENTITY, typename CRITERIA, typename ORDER, typename LIMIT>
        static auto Render(std::ostream& strm, Proxy<select::Query<PROJECTION, ENTITY, CRITERIA, ORDER, LIMIT>>, utility::Constant<PARAMETER> p);

        template <size_t PARAMETER, typename ENTITY, typename... COMPONENTS>
        static auto Render(std::ostream& strm, Proxy<insert::Query<ENTITY, COMPONENTS...>>, utility::Constant<PARAMETER> p);

        template <size_t PARAMETER, typename ENTITY, typename CRITERIA, typename... COMPONENTS>
        static auto Render(std::ostream& strm, Proxy<update::Query<ENTITY, CRITERIA, COMPONENTS...>>, utility::Constant<PARAMETER> p);
        template <size_t PARAMETER, typename COMPONENT>
        static auto Render(std::ostream& strm, Proxy<update::Set<COMPONENT>>, utility::Constant<PARAMETER> p);

        template <size_t PARAMETER, typename ENTITY, typename CRITERIA>
        static auto Render(std::ostream& strm, Proxy<del::Query<ENTITY, CRITERIA>>, utility::Constant<PARAMETER> p);

        // And now implement them all
        template <utility::Literal SEPARATOR, size_t I, typename COMPONENT, typename... COMPONENTS, size_t PARAMETER>
        static auto RenderVaradic(std::ostream& strm, utility::Constant<PARAMETER> paramOffset, utility::Constant<I>, detail::type_list<COMPONENT, COMPONENTS...>) {
            if constexpr (I > 0)
                strm << SEPARATOR.Value;

            auto intermediary = Render(strm, Proxy<COMPONENT> { }, paramOffset);
            if constexpr (sizeof...(COMPONENTS) > 0)
                return RenderVaradic<SEPARATOR>(strm, intermediary, utility::Constant<I + 1> { }, detail::type_list<COMPONENTS...> { });
            else
                return intermediary;
        }

        template <size_t PARAMETER>
        static auto Render(std::ostream& strm, Proxy<Ignore>, utility::Constant<PARAMETER> p) {
            return p;
        }

        template <size_t PARAMETER, utility::Literal NAME, typename TYPE>
        static auto Render(std::ostream& strm, Proxy<Column<NAME, TYPE>>, utility::Constant<PARAMETER> parameterOffset) {
            strm << NAME.ToString();
            return parameterOffset;
        }

        template <size_t PARAMETER, utility::Literal ALIAS, typename COMPONENT>
        static auto Render(std::ostream& strm, Proxy<Alias<ALIAS, COMPONENT>>, utility::Constant<PARAMETER> parameterOffset) {
            auto nextParameterOffset = Render(strm, Proxy<COMPONENT> { }, parameterOffset);
            strm << " AS " << ALIAS.ToString();
            return nextParameterOffset;
        }

        template <size_t PARAMETER, utility::Literal NAME, utility::Literal SCHEMA, typename... COMPONENTS>
        static auto Render(std::ostream& strm, Proxy<Entity<NAME, SCHEMA, COMPONENTS...>>, utility::Constant<PARAMETER> parameterOffset) {
            strm << SCHEMA.ToString() << '.' << NAME.ToString();
            return parameterOffset;
        }

        template <size_t PARAMETER, typename LEFT, typename RIGHT>
        static auto Render(std::ostream& strm, Proxy<Over<LEFT, RIGHT>>, utility::Constant<PARAMETER> parameterOffset) {
            auto nextParameterOffset = Render(strm, Proxy<LEFT> { }, parameterOffset);
            strm << " OVER (";
            auto rightOfs = Render(strm, Proxy<RIGHT> { }, nextParameterOffset);
            strm << ')';
            return rightOfs;
        }

        template <size_t PARAMETER, typename... COMPONENTS>
        static auto Render(std::ostream& strm, Proxy<Projection<COMPONENTS...>>, utility::Constant<PARAMETER> parameterOffset) {
            strm << "SELECT ";
            return RenderVaradic<", ">(strm, parameterOffset, utility::Constant<0> { }, detail::type_list<COMPONENTS...> { });
        }

        template <size_t PARAMETER, utility::Literal FORMAT, typename COMPONENT>
        static auto Render(std::ostream& strm, Proxy<Criteria<FORMAT, COMPONENT>>, utility::Constant<PARAMETER> parameterOffset) {
            std::stringstream temp;
            auto nextParameterOffset = Render(temp, Proxy<COMPONENT> { }, parameterOffset);
            strm << fmt::format(FORMAT.Value, temp.str(), PARAMETER);
            return utility::Constant<nextParameterOffset.value + 1> { };
        }

        template <size_t PARAMETER, utility::Literal TOKEN, typename... COMPONENTS>
        static auto Render(std::ostream& strm, Proxy<ManyCriteria<TOKEN, COMPONENTS...>>, utility::Constant<PARAMETER> parameterOffset) {
            strm << '(';
            auto result = RenderVaradic<TOKEN>(strm, parameterOffset, utility::Constant<0> { }, detail::type_list<COMPONENTS...> { });
            strm << ')';
            return result;
        }

        template <size_t PARAMETER, utility::Literal NAME, typename TYPE, typename... COMPONENTS>
        static auto Render(std::ostream& strm, Proxy<Function<NAME, TYPE, COMPONENTS...>>, utility::Constant<PARAMETER> parameterOffset) {
            strm << NAME.ToString() << '(';
            auto result = RenderVaradic<", ">(strm, parameterOffset, utility::Constant<0> { }, detail::type_list<COMPONENTS...> { });
            strm << ')';
            return result;
        }

        template <size_t PARAMETER, typename COMPONENT>
        static auto Render(std::ostream& strm, Proxy<PartitionBy<COMPONENT>>, utility::Constant<PARAMETER> parameterOffset) {
            strm << "PARTITION BY ";
            return Render(strm, Proxy<COMPONENT> { }, parameterOffset);
        }

        template <size_t PARAMETER, bool ASCENDING, typename COMPONENT>
        static auto Render(std::ostream& strm, Proxy<OrderByClause<ASCENDING, COMPONENT>> clause, utility::Constant<PARAMETER> parameterOffset) {
            auto nextParameterOffset = Render(strm, Proxy<COMPONENT> { }, parameterOffset);
            strm << (ASCENDING ? " ASC" : " DESC");
            return nextParameterOffset;
        }

        template <size_t PARAMETER, typename... COMPONENTS>
        static auto Render(std::ostream& strm, Proxy<OrderBy<COMPONENTS...>>, utility::Constant<PARAMETER> parameterOffset) {
            strm << "ORDER BY ";
            return RenderVaradic<", ">(strm, parameterOffset, utility::Constant<0> { }, detail::type_list<COMPONENTS...> { });
        }

        template <typename T, typename = void> struct SelectProjection { using type = T; };
        template <typename T> struct SelectProjection<T, std::void_t<typename T::as_projection>> { using type = typename T::as_projection; };

        template <size_t PARAMETER, typename PROJECTION, typename FROM, typename CRITERIA, typename ORDER, typename LIMIT>
        static auto Render(std::ostream& strm, Proxy<select::Query<PROJECTION, FROM, CRITERIA, ORDER, LIMIT>>, utility::Constant<PARAMETER> p) {
            auto projectionOffset = Render(strm, Proxy<typename SelectProjection<PROJECTION>::type> { }, p);
            strm << ' ';
            auto entityOffset = Render(strm, Proxy<FROM> { }, projectionOffset);
            strm << ' ';
            auto criteriaOffset = Render(strm, Proxy<CRITERIA> { }, entityOffset);
            strm << ' ';
            auto orderOffset = Render(strm, Proxy<ORDER> { }, criteriaOffset);
            strm << ' ';
            auto limitOffset = Render(strm, Proxy<LIMIT> { }, orderOffset);
            return limitOffset;
        }

        template <size_t PARAMETER, typename PROJECTION>
        static auto Render(std::ostream& strm, Proxy<insert::Value<PROJECTION>>, utility::Constant<PARAMETER> p) {
            strm << "$" << PARAMETER;
            return utility::Constant<PARAMETER + 1> { };
        }

        template <size_t PARAMETER, typename PROJECTION, typename... COMPONENTS>
        static auto Render(std::ostream& strm, Proxy<insert::Query<PROJECTION, COMPONENTS...>>, utility::Constant<PARAMETER> p) {
            strm << "INSERT INTO ";
            auto entityOffset = Render(strm, Proxy<PROJECTION> { }, p);
            strm << " (";
            auto componentOffset = RenderVaradic<", ">(strm, entityOffset, utility::Constant<0> { }, detail::type_list<COMPONENTS...> { });
            strm << ") VALUES (";
            auto valuesOffset = RenderVaradic<", ">(strm, componentOffset, utility::Constant<0> { }, detail::type_list<insert::Value<COMPONENTS>...> { });
            strm << ')';

            return valuesOffset;
        }

        template <size_t PARAMETER, typename ENTITY, typename CRITERIA, typename... COMPONENTS>
        static auto Render(std::ostream& strm, Proxy<update::Query<ENTITY, CRITERIA, COMPONENTS...>>, utility::Constant<PARAMETER> p) {
            strm << "UPDATE ";
            auto entityOffset = Render(strm, Proxy<ENTITY> { }, p);
            strm << " SET ";
            auto componentOffset = RenderVaradic<", ">(strm, entityOffset, utility::Constant<0> { }, detail::type_list<update::Set<COMPONENTS>...> { });
            strm << ' ';
            auto criteriaOffset = Render(strm, Proxy<CRITERIA> { }, componentOffset);
            return criteriaOffset;
        }

        template <size_t PARAMETER, typename COMPONENT>
        static auto Render(std::ostream& strm, Proxy<update::Set<COMPONENT>>, utility::Constant<PARAMETER> p) {
            auto componentOffset = Render(strm, Proxy<COMPONENT> { }, p);
            strm << " = ";
            return Render(strm, Proxy<insert::Value<COMPONENT>> { }, componentOffset);
        }


        template <size_t PARAMETER, typename ENTITY, typename CRITERIA>
        static auto Render(std::ostream& strm, Proxy<del::Query<ENTITY, CRITERIA>>, utility::Constant<PARAMETER> p) {
            strm << "DELETE FROM ";
            auto entityOfs = Render(strm, Proxy<ENTITY> { }, p);
            strm << ' ';
            return Render(strm, Proxy<CRITERIA> { }, entityOfs);
        }

        template <size_t PARAMETER, typename CRITERIA>
        static auto Render(std::ostream& strm, Proxy<Where<CRITERIA>>, utility::Constant<PARAMETER> p) {
            strm << "WHERE ";
            return Render(strm, Proxy<CRITERIA> { }, p);
        }

        // template <size_t PARAMETER, typename PROJECTION, typename ENTITY, typename CRITERIA, utility::Literal ALIAS, typename ADJ>
        // static auto Render(std::ostream& strm, Query<PROJECTION, ENTITY, CRITERIA>::template With<ALIAS, ADJ>, utility::Constant<PARAMETER> p) {
        //     strm << "WITH " << ALIAS.Value << " AS (";
        //     auto projOffset = Render(strm, ADJ { }, p);
        //     strm << " )";
        //     auto queryOffset = Render(strm, Query<PROJECTION, ENTITY, CRITERIA> { }, projOffset);
        //     
        //     return queryOffset;
        // }

        template <size_t PARAMETER, size_t COUNT, size_t OFFSET>
        static auto Render(std::ostream& strm, Proxy<Limit<COUNT, OFFSET>>, utility::Constant<PARAMETER> p) {
            strm << "LIMIT " << COUNT << " OFFSET " << OFFSET;
            return p;
        }

        template <size_t PARAMETER, typename COMPONENT>
        static auto Render(std::ostream& strm, Proxy<From<COMPONENT>>, utility::Constant<PARAMETER> p) {
            strm << "FROM ";
            return Render(strm, Proxy<COMPONENT> { }, p);
        }
    }

    template <typename PROJECTION, typename ENTITY, typename CRITERIA, typename ORDER, typename LIMIT>
    std::string select::Query<PROJECTION, ENTITY, CRITERIA, ORDER, LIMIT>::Render() {
        std::stringstream ss;
        renderer::Render(ss, renderer::Proxy<select::Query<PROJECTION, ENTITY, CRITERIA, ORDER, LIMIT>> { }, utility::Constant<1> { });
        return ss.str();
    }

    template <typename ENTITY, typename... COMPONENTS>
    std::string insert::Query<ENTITY, COMPONENTS...>::Render() {
        std::stringstream ss;
        renderer::Render(ss, renderer::Proxy<insert::Query<ENTITY, COMPONENTS...>> { }, utility::Constant<1> { });
        return ss.str();
    }

    template <typename ENTITY, typename CRITERIA, typename... COMPONENTS>
    std::string update::Query<ENTITY, CRITERIA, COMPONENTS...>::Render() {
        std::stringstream ss;
        renderer::Render(ss, renderer::Proxy<update::Query<ENTITY, CRITERIA, COMPONENTS...>> { }, utility::Constant<1> { });
        return ss.str();
    }

    template <typename ENTITY, typename CRITERIA>
    std::string del::Query<ENTITY, CRITERIA>::Render() {
        std::stringstream ss;
        renderer::Render(ss, renderer::Proxy<del::Query<ENTITY, CRITERIA>> { }, utility::Constant<1> { });
        return ss.str();
    }
}

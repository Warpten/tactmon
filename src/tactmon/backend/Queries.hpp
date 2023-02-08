#ifndef backend_queries_hpp__
#define backend_queries_hpp__

#include <cstdint>
#include <optional>
#include <string>
#include <sstream>
#include <type_traits>
#include <vector>

#include <pqxx/pqxx>

#include <ext/Literal.hpp>

namespace backend::db {
    template <typename...> struct TypeList { };
    template <auto...> struct ValueList { };

    template <ext::Literal, typename> struct Column;

    namespace detail {
        template <typename T> struct is_optional : std::false_type { };
        template <typename T> struct is_optional<std::optional<T>> : std::true_type { };

        template <size_t I, ext::Literal N, typename T>
        struct column_storage {
            explicit column_storage(pqxx::row& row) : _value(row[I].as<T>()) { }

            template <size_t X, typename = std::enable_if_t<I == X>>
            T const& get() const { return _value; }

            template <ext::Literal X, typename = std::enable_if_t<N::ToString() == X::ToString()>>
            T const& get() const { return _value; }

            template <typename C, typename = std::enable_if_t<std::is_same_v<C, Column<N, T>>>>
            T const& get() const { return _value; }

            T const& value() const { return _value; }

        private:
            T _value;
        };

        template <typename, typename, typename> struct make_column_storage;
        template <size_t... Is, ext::Literal... Ns, typename... Ts>
        struct make_column_storage<std::index_sequence<Is...>, ValueList<Ns...>, TypeList<Ts...>>
            : public column_storage<Is, Ns, Ts>...
        {
            explicit make_column_storage(pqxx::row& row) : column_storage<Is, Ns, Ts>(row)...
            {
            }

        };
    }

    /**
     * Models a SQL column.
     * 
     * @typeparam N The name of the column.
     * @typeparam T The type of the column.
     */
    template <ext::Literal N, typename T>
    struct Column {
        using Type = T;
        constexpr static const ext::Literal Name = N;

        static void Render(std::ostream& strm) {
            strm << Name.Value;
        }
    };

    namespace detail {
        template <typename T> struct is_column : std::false_type { };
        template <ext::Literal N, typename T> struct is_column<Column<N, T>> : std::true_type { };
    }

    /**
    * Models a SQL projection.
    * 
    * @typeparam Ts... A sequence of types modeling a SQL `Column`.
    */
    template <typename... Ts>
    struct Projection : public detail::make_column_storage<
        std::make_index_sequence<sizeof...(Ts)>,
        ValueList<Ts::Name...>,
        TypeList<typename Ts::Type...>
    > {
        explicit Projection(pqxx::row& row) : detail::make_column_storage<
            std::make_index_sequence<sizeof...(Ts)>, 
            ValueList<Ts::Name...>, 
            TypeList<typename Ts::Type...>
        >(row) { }

        static void RenderProjection(std::ostream& strm) {
            [&] <size_t... Is, typename... Xs>(std::index_sequence<Is...>, TypeList<Xs...>) {
                ([&] <size_t I, typename X>(std::integral_constant<size_t, I>, X) {
                    if constexpr (I > 0)
                        strm << ", ";

                    X::Render(strm);
                }(std::integral_constant<size_t, Is> { }, Xs { }), ...);
            }(std::index_sequence_for<Ts...> { }, TypeList<Ts...> { });
        }
    };

    template <size_t I, ext::Literal N, typename T>
    auto get(detail::column_storage<I, N, T> const& store) -> T const& { return store.value(); }
    template <size_t I, ext::Literal N, typename T>
    auto get(detail::column_storage<I, N, T>& store) -> T const& { return store.value(); }
    template <size_t I, ext::Literal N, typename T>
    auto get(detail::column_storage<I, N, T>&& store) -> T const& { return store.value(); }

    template <ext::Literal N, typename T, size_t I>
    auto get(detail::column_storage<I, N, T> const& store) -> T const& { return store.value(); }
    template <ext::Literal N, typename T, size_t I>
    auto get(detail::column_storage<I, N, T>& store) -> T const& { return store.value(); }
    template <ext::Literal N, typename T, size_t I>
    auto get(detail::column_storage<I, N, T>&& store) -> T const& { return store.value(); }

    template <typename C, typename... Cs, typename = std::enable_if_t<detail::is_column<C>::value>>
    auto get(Projection<Cs...> const& store) { return get<C::Name>(store); }
    template <typename C, typename... Cs, typename = std::enable_if_t<detail::is_column<C>::value>>
    auto get(Projection<Cs...>& store) { return get<C::Name>(store); }
    template <typename C, typename... Cs, typename = std::enable_if_t<detail::is_column<C>::value>>
    auto get(Projection<Cs...>&& store) { return get<C::Name>(store); }

    /**
     * Models a SQL entity.
     * 
     * @typeparam N     The name of the entity.
     * @typeparam Ts... A sequence of types modeling a SQL `Column`.
     */
    template <ext::Literal N, typename... Ts>
    requires (detail::is_column<Ts>::value && ...)
    struct Entity : Projection<Ts...> {
        explicit Entity(pqxx::row& row) : Projection<Ts...>(row) { }

        constexpr static const ext::Literal Name = N;

        static void RenderTableName(std::ostream& strm) {
            strm << Name.Value;
        }

    };

    template <ext::Literal S, typename E>
    struct Schema final : E {
        static void RenderTableName(std::ostream& strm) {
            strm << S.Value << ".";
            E::RenderTableName(strm);
        }
    };

    /**
     * Models a SQL criteria.
     * 
     * @typeparam F The format string used to render the criteria.
     * @typeparam T An object modeling a SQL `Column`.
     */
    template <ext::Literal F, typename T>
    requires detail::is_column<T>::value
    struct Criterion final {
        using Arguments = std::tuple<typename T::Type>;

        template <size_t I, size_t A>
        static void Render(std::ostream& strm) {
            strm << std::format(F.Value, T::Name.Value, A);
        }

        constexpr static const size_t Size = 1;
    };

    template <typename C> using Equals = Criterion<"{} = ${}", C>;
    template <typename C> using Greater = Criterion<"{} > ${}", C>;
    template <typename C> using Less = Criterion<"{} < ${}", C>;
    template <typename C> using GreaterOrEqual = Criterion<"{} >= ${}", C>;
    template <typename C> using LessOrEqual = Criterion<"{} <= ${}", C>;
    template <typename C> using NotEquals = Criterion<"{} != ${}", C>;

    /**
     * Models a SQL criteria that aggregates various criterias.
     * 
     * @typeparam Token The token to use when joining inner criterias.
     * @typeparam Cs... A sequence of types modeling a SQL `Criteria`.
     */
    template <ext::Literal Token, typename... Cs>
    struct VarargCriterion final {
        using Arguments = decltype(std::tuple_cat(typename Cs::Arguments{ }...));

        template <size_t I, size_t A>
        static void Render(std::ostream& strm) {
            strm << '(';
            Render_<0, A>(strm);
            strm << ')';
        }

        constexpr static const size_t Size = sizeof...(Cs);

    private:
        template <size_t I>
        using criterion_type = std::tuple_element_t<I, std::tuple<Cs...>>;

        template <size_t I, size_t A>
        static void Render_(std::ostream& strm) {
            using criterion_itr = criterion_type<I>;

            criterion_itr::template Render<0, A>(strm);
            if constexpr (I + 1 < sizeof...(Cs)) {
                strm << Token.Value;
                Render_<I + 1, A + criterion_itr::Size>(strm);
            }
        }
    };

    template <typename... Cs> using And = VarargCriterion<" AND ", Cs...>;
    template <typename... Cs> using Or = VarargCriterion<" OR ", Cs...>;

    template <size_t I, size_t O> struct Limit {
        static void Render(std::ostream& strm) {
            strm << " LIMIT " << I << " OFFSET " << O;
        }
    };

    struct All { static void Render(std::ostream&) { } };

    /**
     * Models a SQL SELECT query.
     */
    template <typename P, typename E, typename C, typename L>
    struct Select {
        using Entity = E;
        using Arguments = typename C::Arguments;
        using Projection = P;

        static std::string Render() {
            return Render_().str();
        }

    private:
        static std::stringstream Render_() {
            std::stringstream query;
            query << "SELECT ";
            P::RenderProjection(query);
            query << " FROM ";
            E::RenderTableName(query);
            if constexpr (C::Size > 0) {
                query << " WHERE ";
                C::template Render<0, 1>(query);
            }
            L::Render(query);

            return query;
        }
    };

    template <ext::Literal Identifier, typename Query>
    struct PreparedStatement final {
        using Arguments = typename Query::Arguments;

        static void Prepare(pqxx::connection& connection) {
            connection.prepare(pqxx::zview{ Identifier.Value, Identifier.Size }, Query::Render());
        }

        static void Update(pqxx::transaction_base& transaction, Arguments&& args) {
            Execute(transaction, std::forward<Arguments>(args));
        }

        static std::vector<typename Query::Projection> Execute(pqxx::transaction_base& transaction, Arguments&& args) {
            using Projection = typename Query::Projection;

            pqxx::result resultSet = Execute_(transaction, pqxx::zview { Identifier.Value, Identifier.Size }, args);

            std::vector<Projection> entities;
            for (pqxx::row row : resultSet)
                entities.emplace_back(row);
            return entities;
        }

        static std::optional<typename Query::Projection> ExecuteOne(pqxx::transaction_base& transaction, Arguments&& args) {
            using Projection = typename Query::Projection;

            pqxx::row resultSet = ExecuteOne_(transaction, pqxx::zview { Identifier.Value, Identifier.Size }, args);
            if (resultSet.empty())
                return std::nullopt;

            return Projection { resultSet };
        }
    private:

        template <typename... Args>
        static auto Execute_(pqxx::transaction_base& transaction, pqxx::zview requestIdentifier, std::tuple<Args...> args) {
            return std::apply([&](auto... arg) mutable {
                return transaction.exec_prepared(requestIdentifier, arg...);
            }, args);
        }

        template <typename... Args>
        static auto ExecuteOne_(pqxx::transaction_base& transaction, pqxx::zview requestIdentifier, std::tuple<Args...> args) {
            return std::apply([&](auto... arg) mutable {
                return transaction.exec_prepared1(requestIdentifier, arg...);
            }, args);
        }
    };
}

#endif // backend_queries_hpp__

#ifndef backend_queries_hpp__
#define backend_queries_hpp__

#include <pqxx/pqxx>

#include <cstdint>
#include <format>
#include <optional>
#include <string>
#include <sstream>
#include <type_traits>
#include <vector>

#include <ext/Literal.hpp>

namespace backend::db {
    namespace detail {
        template <typename T> struct get_value_type { };
        template <typename T> struct get_column_type { };
    }

    template <typename... Ts> struct TypeList {
        constexpr static const size_t Size = sizeof...(Ts);
    };

    template <auto...> struct ValueList { };
    template <ext::Literal, typename> struct Column;
    template <ext::Literal, typename> struct Alias;
    template <typename> struct Count;

    /**
    * Models a SQL column.
    * 
    * @typeparam N The name of the column.
    * @typeparam T The type of the column.
    */
    template <ext::Literal N, typename T>
    struct Column {
        using value_type = T;
        constexpr static const ext::Literal Name = N;

        Column() = default;

        static void Render(std::ostream& strm) {
            strm << Name.Value;
        }
    };

    namespace detail {
        template <ext::Literal N, typename T>
        struct get_value_type<Column<N, T>> {
            using type = T;
        };

        template <ext::Literal N, typename T>
        struct get_column_type<Column<N, T>> {
            using type = Column<N, T>;
        };
    }

    template <typename C>
    struct Count {
        Count() = default;

        static void Render(std::ostream& strm) {
            strm << "COUNT(";
            C::Render(strm);
            strm << ")";
        }
    };

    namespace detail {
        template <typename C>
        struct get_value_type<Count<C>> {
            using type = typename get_value_type<C>::type;
        };

        template <typename C>
        struct get_column_type<Count<C>> {
            using type = typename get_column_type<C>::type;
        };
    }

    template <typename C, typename P>
    struct Over {
        Over() = default;

        static void Render(std::ostream& strm) {
            C::Render(strm);
            strm << " OVER (";
            P::Render(strm);
            strm << ')';
        }
    };

    namespace detail {
        template <typename C, typename P>
        struct get_value_type<Over<C, P>> {
            using type = typename get_value_type<C>::type;
        };

        template <typename C, typename P>
        struct get_column_type<Over<C, P>> {
            using type = typename get_column_type<C>::type;
        };
    }

    template <ext::Literal A, typename C>
    struct Alias {
        Alias() = default;

        static void Render(std::ostream& strm) {
            C::Render(strm);
            strm << " AS " << A.Value;
        }
    };

    namespace detail {
        template <ext::Literal A, typename C>
        struct get_value_type<Alias<A, C>> {
            using type = typename get_value_type<C>::type;
        };

        template <ext::Literal A, typename C>
        struct get_column_type<Alias<A, C>> {
            using type = typename get_column_type<C>::type;
        };

        template <typename T> struct is_alias : std::false_type { };
        template <ext::Literal A, typename C> struct is_alias<Alias<A, C>> : std::true_type { };
    }

    template <typename C>
    struct Partition {
        static void Render(std::ostream& strm) {
            strm << "PARTITION BY ";
            C::Render(strm);
        }
    };

    namespace detail {
        template <typename T> struct is_optional : std::false_type { };
        template <typename T> struct is_optional<std::optional<T>> : std::true_type { };

        template <size_t I, typename T /* Column<..., ...> */>
        struct column_storage : public T {
            explicit column_storage(pqxx::row& row) : T(), _value(row[I].as<typename detail::get_value_type<T>::type>()) { }

            typename detail::get_value_type<T>::type const& value() const& { return _value; }
            typename detail::get_value_type<T>::type value() & { return _value;}
            typename detail::get_value_type<T>::type value() && { return _value; }

        private:
            typename detail::get_value_type<T>::type _value;
        };

        template <size_t I, typename T>
        struct get_column_type<column_storage<I, T>> {
            using type = typename get_column_type<T>::type;
        };

        template <size_t I, typename T>
        struct get_value_type<column_storage<I, T>> {
            using type = typename get_value_type<T>::type;
        };

        template <typename, typename...> struct make_column_storage;
        template <size_t... Is, ext::Literal... Ns, typename... Ts, template <ext::Literal, typename> typename... Cs>
        struct make_column_storage<std::index_sequence<Is...>, Cs<Ns, Ts>...>
            : public column_storage<Is, Cs<Ns, Ts>>...
        {
            explicit make_column_storage(pqxx::row& row) : column_storage<Is, Cs<Ns, Ts>>(row)...
            { }
        };

        // Helps the compiler slice on invocation. Effectively does nothing.
        template <typename C, size_t I>
        detail::column_storage<I, C>& slice(detail::column_storage<I, C>& store) { return store; }
        template <typename C, size_t I>
        detail::column_storage<I, C>&& slice(detail::column_storage<I, C>&& store) { return store; }
        template <typename C, size_t I>
        detail::column_storage<I, C> const& slice(detail::column_storage<I, C> const& store) { return store; }

    }

    namespace detail {
        template <typename T> struct is_column : std::false_type { };
        template <ext::Literal N, typename T> struct is_column<Column<N, T>> : std::true_type { };

        template <typename X, typename T> struct of_type;
        template <size_t I, typename T> struct at_index;
    }

    /**
    * Models a SQL projection.
    * 
    * @typeparam Ts... A sequence of types modeling a SQL `Column`.
    */
    template <typename... Ts>
    struct Projection : private detail::make_column_storage<
        std::make_index_sequence<sizeof...(Ts)>,
        Ts...
    > {
        using Base = detail::make_column_storage<
            std::make_index_sequence<sizeof...(Ts)>,
            Ts...
        >;

        explicit Projection(pqxx::row& row);

        static void RenderProjection(std::ostream& strm) {
            [&] <size_t... Is, typename... Xs>(std::index_sequence<Is...>, TypeList<Xs...>) {
                ([&] <size_t I, typename X>(std::integral_constant<size_t, I>, X) {
                    if constexpr (I > 0)
                        strm << ", ";

                    X::Render(strm);
                }(std::integral_constant<size_t, Is> { }, Xs{ }), ...);
            }(std::index_sequence_for<Ts...> { }, TypeList<Ts...> { });
        }

    private:
        template <size_t, typename> friend struct detail::at_index;
        template <typename, typename> friend struct detail::of_type;

        template <typename C, typename T>
        friend auto get(T&& store)
            -> typename detail::get_value_type<
                typename detail::of_type<C, T>::type
            >::type;
        
        template <size_t I, typename T>
        friend auto get(T&& store)
            -> typename detail::get_value_type<
                typename detail::at_index<I, T>::type
            >::type;
    };

    namespace detail {
        template <size_t I, typename T>
        struct at_index {
            template <typename U>
            static auto select(detail::column_storage<I, U> column) -> detail::column_storage<I, U> { return column; }

            using type = decltype(select(std::declval<T>()));
        };

        template <typename X, typename T>
        struct of_type {
            template <size_t I>
            static auto select(detail::column_storage<I, X> column) -> detail::column_storage<I, X> { return column; }

            using type = decltype(select(std::declval<T>()));
        };
    }

    template <size_t I, typename T>
    auto get(T&& store) -> typename detail::get_value_type<typename detail::at_index<I, T>::type>::type {
        return detail::at_index<I, std::decay_t<T>>::select(store).value();
    }

    template <typename C, typename T>
    auto get(T&& store) -> typename detail::get_value_type<typename detail::of_type<C, T>::type>::type{
        return detail::of_type<C, T>::select(store).value();
    }

    /**
    * Models a SQL entity.
    * 
    * @typeparam N     The name of the entity.
    * @typeparam Ts... A sequence of types modeling a SQL `Column`.
    */
    template <ext::Literal N, typename... Ts>
        requires (detail::is_column<Ts>::value && ...)
    struct Entity : public Projection<Ts...> {
        explicit Entity(pqxx::row& row);

        constexpr static const ext::Literal Name = N;

        static void RenderTableName(std::ostream& strm) {
            strm << Name.Value;
        }
    };

    template <typename... Ts>
    Projection<Ts...>::Projection(pqxx::row& row) : Base(row) { }

    template <ext::Literal N, typename... Ts>
    requires (detail::is_column<Ts>::value && ...)
    Entity<N, Ts...>::Entity(pqxx::row& row) : Projection<Ts...>(row) { }

    namespace {
        template <typename...>
        struct if_you_see_this_it_means_the_column_youre_looking_for_is_not_part_of_the_projection : std::false_type { };
    }

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
        using Arguments = std::tuple<typename T::value_type>;

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
    struct Ignore { static void Render(std::ostream&) { } };

    enum class OrderKind : uint8_t{
        Descending,
        Ascending
    };

    template <OrderKind O, typename C>
    struct Order {
        static void Render(std::ostream& strm) {
            strm << " ORDER BY ";
            C::Render(strm);
            if constexpr (O == OrderKind::Descending)
                strm << " DESC";
            else
                strm << " ASC";
        }
    };

    /**
    * Models a SQL SELECT query.
    * 
    * @typeparam P The projection type.
    * @typeparam E The entity type.
    * @typeparam C The query condition of this query.
    * @typeparam O The order clause of this query.
    * @typeparam L The limit on the amount of results of this query.
    */
    template <typename P, typename E, typename C, typename O = Ignore, typename L = Ignore>
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
            O::Render(query);
            query << ' ';
            L::Render(query);

            return query;
        }
    };

    template <ext::Literal Identifier, typename Query>
    struct PreparedStatement final {
        using Arguments = typename Query::Arguments;

        static void Prepare(pqxx::connection& connection) {
            connection.prepare(pqxx::zview{ Identifier.Value, Identifier.Size - 1 }, Query::Render());
        }

        static void Update(pqxx::transaction_base& transaction, Arguments&& args) {
            Execute(transaction, std::forward<Arguments>(args));
        }

        static std::vector<typename Query::Projection> Execute(pqxx::transaction_base& transaction, Arguments&& args) {
            using Projection = typename Query::Projection;

            pqxx::result resultSet = Execute_(transaction, pqxx::zview { Identifier.Value, Identifier.Size - 1 }, args);

            std::vector<Projection> entities;
            entities.reserve(resultSet.size());
            for (pqxx::row row : resultSet)
                entities.emplace_back(row);
            return entities;
        }

        static std::optional<typename Query::Projection> ExecuteOne(pqxx::transaction_base& transaction, Arguments&& args) {
            using Projection = typename Query::Projection;

            pqxx::row resultSet = ExecuteOne_(transaction, pqxx::zview { Identifier.Value, Identifier.Size - 1 }, args);
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

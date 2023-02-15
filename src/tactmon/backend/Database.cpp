#include "backend/Database.hpp"

#include <ext/Literal.hpp>

#include <format>
#include <optional>
#include <sstream>
#include <tuple>
#include <type_traits>
#include <vector>

#include <pqxx/pqxx>

namespace backend {
    Database::Database(boost::asio::io_context::strand strand, std::string_view username, std::string_view password, std::string_view host, uint64_t port, std::string_view name)
        : _connection(std::format("user={} password={} host={} port={} dbname={} target_session_attrs=read-write", username, password, host, port, name))
        , builds(strand, _connection)
    {
    }
}

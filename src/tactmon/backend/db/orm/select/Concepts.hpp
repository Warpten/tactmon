#pragma once

#include "backend/db/orm/concepts/Concepts.hpp"
#include "backend/db/orm/select/CTE.hpp"

namespace backend::db::orm::select {
    namespace concepts {
        namespace detail {
            template <typename T> struct IsCTE : std::false_type { };
            template <utility::Literal ALIAS, bool RECURSIVE, orm::concepts::StreamRenderable QUERY>
            struct IsCTE<CTE<ALIAS, RECURSIVE, QUERY>> : std::true_type { };
        }

        template <typename T>
        concept IsCTE = detail::IsCTE<T>::value;
    }
}

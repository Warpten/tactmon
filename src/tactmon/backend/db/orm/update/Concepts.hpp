#pragma once

#include "backend/db/orm/Concepts.hpp"
#include "backend/db/orm/update/CTE.hpp"

namespace backend::db::orm::select {
    namespace concepts {
        namespace detail {
            template <typename T> struct IsSet : std::false_type { };
            template <orm::concepts::StreamRenderable... ASSIGNMENTS...> struct IsSet<Set<ASSIGNMENTS...>> : std::true_type { };
        }

        template <typename T>
        concept IsSet = detail::IsSet<T>::value;
    }
}

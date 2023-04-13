#pragma once

#include "backend/db/orm/Concepts.hpp"
#include "backend/db/orm/update/Set.hpp"

namespace backend::db::update {
    namespace concepts {
        namespace detail {
            template <typename T> struct IsSet : std::false_type { };
            template <typename... ASSIGNMENTS> struct IsSet<Set<ASSIGNMENTS...>> : std::true_type { };
        }

        template <typename T>
        concept IsSet = detail::IsSet<T>::value;
    }
}

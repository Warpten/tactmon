#pragma once

namespace backend::db::orm {
    struct QueryGenerator {
        QueryGenerator() { }

        void Append(std::string_view value) {
            _value += value;
        }



    private:
        std::string _value;
        std::unordered_map<uint32_t, std::string> _parameterNames;
    };
}

#pragma once

#include <string>

namespace tigl {
    class TypeSystem;
    class Tables;

    void genCode(const std::string& outputLocation, const TypeSystem& types, const Tables& tables);
}

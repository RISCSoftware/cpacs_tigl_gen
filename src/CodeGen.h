#pragma once

#include <string>

namespace tigl {
    class TypeSystem;
    class Tables;

    void genCode(const std::string& outputLocation, TypeSystem& types, Tables& tables);
}

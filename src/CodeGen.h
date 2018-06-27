#pragma once

#include <string>

namespace tigl {
    struct TypeSystem;
    class Tables;

    void genCode(const std::string& outputLocation, TypeSystem typeSystem, const std::string& ns, const Tables& tables);
}

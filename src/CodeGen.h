#pragma once

#include <string>

#include "TypeSystem.h"

namespace tigl {
    void genCode(const std::string& outputLocation, TypeSystem typeSystem, const std::string& ns, const Tables& tables);
}

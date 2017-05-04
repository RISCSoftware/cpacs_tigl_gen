#pragma once

#include <string>

#include "TypeSystem.h"

namespace tigl {
    void genCode(const std::string& outputLocation, TypeSystem typeSystem, const Tables& tables);
}

#pragma once

#include <string>
#include <algorithm>
#include <unordered_map>
#include <vector>
#include <stdexcept>

#include "Variant.hpp"
#include "SchemaParser.h"

namespace tigl {
    struct Attribute;
    struct Element;
    struct SimpleContent;
    struct ComplexType;
    struct SimpleType;
    class SchemaParser;
    class Tables;

    enum class Cardinality {
        Optional,
        Mandatory,
        Vector
    };

    inline auto toString(const Cardinality& c) -> std::string {
        switch (c) {
            case Cardinality::Optional:  return "Optional";
            case Cardinality::Mandatory: return "Mandatory";
            case Cardinality::Vector:    return "Vector";
            default: throw std::logic_error("No toString defined");
        }
    }

    enum class XMLConstruct {
        Element,
        Attribute,
        SimpleContent,
        FundamentalTypeBase
    };

    struct Class;
    struct Enum;

    struct Field {
        std::string originXPath;
        std::string cpacsName;
        std::string typeName;
        XMLConstruct xmlType;
        Cardinality cardinality;

        std::string customName;

        auto name() const -> std::string {
            auto name = [this] {
                if (!customName.empty())
                    return customName;
                else
                    return cpacsName;
            }();
            if (cardinality == Cardinality::Vector)
                name += 's';
            return name;
        }

        auto fieldName() const -> std::string {
            return "m_" + name();
        }
    };

    // non owning
    struct ClassDependencies {
        // in the context of inheritance
        std::vector<Class*> bases;
        std::vector<Class*> deriveds;

        // in the context of the XML tree
        std::vector<Class*> parents;
        std::vector<Class*> children;
        std::vector<Enum*> enumChildren;
    };

    struct Class {
        std::string originXPath;
        std::string name;
        std::string base;
        std::vector<Field> fields;
        std::string choiceExpression;
        bool pruned = false;

        ClassDependencies deps;
    };

    struct EnumValue {
        std::string cpacsName;
        std::string customName;

        EnumValue() = default;
        EnumValue(const std::string& name)
            : cpacsName(name) {}

        auto name() const -> std::string {
            if (!customName.empty())
                return customName;
            else
                return cpacsName;
        }

        friend auto operator==(const EnumValue& a, const EnumValue& b) -> bool {
            return a.name() == b.name();
        }
    };

    struct EnumDependencies {
        std::vector<Class*> parents;
    };

    struct Enum {
        std::string originXPath;
        std::string name;
        std::vector<EnumValue> values;
        bool pruned = false;

        EnumDependencies deps;
    };

    struct TypeSystem {
        std::unordered_map<std::string, Class> classes;
        std::unordered_map<std::string, Enum> enums;
    };

    auto buildTypeSystem(SchemaTypes types, const Tables& tables) -> TypeSystem;

    void writeGraphVisFile(const TypeSystem& ts, const std::string& typeSystemGraphVisFile);
}

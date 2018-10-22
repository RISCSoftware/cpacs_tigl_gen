#pragma once

#include <boost/variant/recursive_wrapper.hpp>

#include <map>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>
#include <stdexcept>

#include "Variant.hpp"
#include "SchemaParser.h"

namespace tigl {
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
        unsigned int minOccurs = 0;
        unsigned int maxOccurs = 0;
        std::string defaultValue;

        std::string namePostfix;

        auto cardinality() const -> Cardinality {
            if (minOccurs == 0 && maxOccurs == 1)
                return Cardinality::Optional;
            if (minOccurs == 1 && maxOccurs == 1)
                return Cardinality::Mandatory;
            if (minOccurs >= 0 && maxOccurs > 1)
                return Cardinality::Vector;
            throw std::runtime_error("Invalid cardinalities, min: " + std::to_string(minOccurs) + ", max: " + std::to_string(maxOccurs));
        }

        auto name() const -> std::string {
            auto n = cpacsName;
            // append "s" to vector fields
            if (cardinality() == Cardinality::Vector && !cpacsName.empty() && cpacsName.back() != 's')
                n += "s";
            return n + namePostfix;
        }

        auto nameWithoutVectorS() const -> std::string {
            return cpacsName + namePostfix;
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

    struct ChoiceElement {
        std::size_t index;
        bool optionalBefore;
    };

    struct Choice;
    using ChoiceElements = std::vector<boost::variant<ChoiceElement, boost::recursive_wrapper<Choice>>>;

    struct Choice {
        std::vector<ChoiceElements> options;
    };

    struct Class {
        std::string originXPath;
        std::string name;
        std::string base;
        std::vector<Field> fields;
        ChoiceElements choices;
        bool pruned = false;

        ClassDependencies deps;
    };

    struct EnumValue {
        std::string cpacsName;
        std::string customName;

        EnumValue() = default;
        EnumValue(std::string name)
            : cpacsName(std::move(name)) {}

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
        std::map<std::string, Enum> enums;
    };

    auto buildTypeSystem(xsd::SchemaTypes types, const Tables& tables) -> TypeSystem;

    void writeGraphVisFile(const TypeSystem& ts, const std::string& typeSystemGraphVisFile);
}

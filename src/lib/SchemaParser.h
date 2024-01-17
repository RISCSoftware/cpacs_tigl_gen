#pragma once


#include <string>
#include <vector>
#include <limits>
#include <unordered_map>

#include "TixiDocument.h"
#include "Variant.hpp"

namespace tigl {
    namespace xsd {
        constexpr int unbounded = std::numeric_limits<unsigned int>::max();

        struct XSDElement {
            std::string xpath;
        };

        struct Attribute : XSDElement {
            std::string name;
            std::string type;
            std::string defaultValue;
            std::string fixed;
            bool optional;
            std::string documentation;
        };

        struct Element : XSDElement {
            std::string name;
            std::string type;
            unsigned int minOccurs;
            unsigned int maxOccurs;
            std::string defaultValue;
            std::string documentation;
        };

        struct Any : XSDElement {

        };

        struct Group : XSDElement {

        };

        struct All : XSDElement {
            std::vector<Element> elements;
        };

        struct Choice;
        struct Sequence : XSDElement {
            std::vector<Variant<Element, Group, Choice, Sequence, Any>> elements;
        };

        struct Choice : XSDElement {
            std::vector<Variant<Element, Group, Choice, Sequence, Any>> elements;
            unsigned int minOccurs;
        };

        struct SimpleContent : XSDElement {
            std::string type;
        };

        struct Type : XSDElement {
            std::string name;
            std::string base;
        };

        struct ComplexType : Type {
            Variant<Group, All, Choice, Sequence, SimpleContent, Any> content;
            std::vector<Attribute> attributes;
            std::string documentation;
        };

        struct SimpleType : Type {
            std::vector<std::string> restrictionValues;
        };

        struct SchemaTypes {
            std::vector<std::string> roots;
            std::unordered_map<std::string, Variant<ComplexType, SimpleType>> types;
        };

        auto parseSchema(const std::string& cpacsLocation) -> SchemaTypes;

        auto stripTypeSuffix(const std::string& name) -> std::string;
        auto stripSimpleContentSuffix(const std::string& name) -> std::string;
    }
}

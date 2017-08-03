#include <iostream>

#include "NotImplementedException.h"
#include "SchemaParser.h"

namespace tigl {
    namespace xsd {
        namespace {
            const std::string c_simpleContentTypeSuffx = "_SimpleContentType";
        }

        class SchemaParser {
        public:
            SchemaParser(const std::string& cpacsLocation)
                : document(tixihelper::TixiDocument::createFromFile(cpacsLocation)) {
                document.registerNamespace("http://www.w3.org/2001/XMLSchema", "xsd");

                document.forEachChild("/xsd:schema/xsd:simpleType", [&](const std::string& xpath) {
                    readSimpleType(xpath);
                });

                document.forEachChild("/xsd:schema/xsd:complexType", [&](const std::string& xpath) {
                    readComplexType(xpath);
                });

                document.forEachChild("/xsd:schema/xsd:element", [&](const std::string& xpath) {
                    m_types.roots.push_back(readElement(xpath).type);
                });
            }

            Group readGroup(const std::string& xpath, const std::string& containingTypeName) {
                throw NotImplementedException("XSD group is not implemented");
                return Group();
            }

            All readAll(const std::string& xpath, const std::string& containingTypeName) {
                // <all
                // id = ID
                // maxOccurs = 1
                // minOccurs = 0 | 1
                // any attributes
                // >
                // (annotation? , element*)
                // </all>
                All all;
                all.xpath = xpath;
                document.forEachChild(xpath + "/xsd:element", [&](const std::string& xpath) {
                    all.elements.push_back(readElement(xpath, containingTypeName));
                });
                return all;
            }

            Choice readChoice(const std::string& xpath, const std::string& containingTypeName) {
                // <choice
                // id=ID
                // maxOccurs=nonNegativeInteger|unbounded
                // minOccurs=nonNegativeInteger
                // any attributes
                // >
                // (annotation?,(element|group|choice|sequence|any)*)
                // </choice>
                Choice ch;
                ch.xpath;
                document.forEachChild(xpath + "/xsd:element", [&](const std::string& xpath) {
                    ch.elements.push_back(readElement(xpath, containingTypeName));
                });
                document.forEachChild(xpath + "/xsd:group", [&](const std::string& xpath) {
                    ch.elements.push_back(readGroup(xpath, containingTypeName));
                });
                document.forEachChild(xpath + "/xsd:choice", [&](const std::string& xpath) {
                    ch.elements.push_back(readChoice(xpath, containingTypeName));
                });
                document.forEachChild(xpath + "/xsd:sequence", [&](const std::string& xpath) {
                    ch.elements.push_back(readSequence(xpath, containingTypeName));
                });
                document.forEachChild(xpath + "/xsd:any", [&](const std::string& xpath) {
                    ch.elements.push_back(readAny(xpath, containingTypeName));
                });
                return ch;
            }

            Sequence readSequence(const std::string& xpath, const std::string& containingTypeName) {
                // <sequence
                // id=ID
                // maxOccurs=nonNegativeInteger|unbounded
                // minOccurs=nonNegativeInteger
                // any attributes
                // >
                // (annotation?,(element|group|choice|sequence|any)*)
                // </sequence>
                Sequence seq;
                seq.xpath = xpath;
                document.forEachChild(xpath + "/xsd:element", [&](const std::string& xpath) {
                    seq.elements.push_back(readElement(xpath, containingTypeName));
                });
                document.forEachChild(xpath + "/xsd:group", [&](const std::string& xpath) {
                    seq.elements.push_back(readGroup(xpath, containingTypeName));
                });
                document.forEachChild(xpath + "/xsd:choice", [&](const std::string& xpath) {
                    seq.elements.push_back(readChoice(xpath, containingTypeName));
                });
                document.forEachChild(xpath + "/xsd:sequence", [&](const std::string& xpath) {
                    seq.elements.push_back(readSequence(xpath, containingTypeName));
                });
                document.forEachChild(xpath + "/xsd:any", [&](const std::string& xpath) {
                    seq.elements.push_back(readAny(xpath, containingTypeName));
                });
                return seq;
            }

            Any readAny(const std::string& xpath, const std::string& containingTypeName) {
                // <any
                // id=ID
                // maxOccurs=nonNegativeInteger|unbounded
                // minOccurs=nonNegativeInteger
                // namespace=namespace
                // processContents=lax|skip|strict
                // any attributes
                // >
                // (annotation?)
                // </any>
                throw NotImplementedException("XSD any is not implemented");
                return Any();
            }

            void readExtension(const std::string& xpath, ComplexType& type) {
                type.base = document.textAttribute(xpath, "base");
                readComplexTypeElementConfiguration(xpath, type);
            }

            void readSimpleContent(const std::string& xpath, ComplexType& type) {
                // <simpleContent
                // id=ID
                // any attributes
                // >
                // (annotation?,(restriction|extension))
                // </simpleContent>

                SimpleContent sc;
                sc.xpath = xpath;
                if (document.checkElement(xpath + "/xsd:restriction")) {
                    if (document.checkElement(xpath + "/xsd:restriction/xsd:enumeration")) {
                        // generate an additional type for this enum
                        SimpleType stype;
                        stype.xpath = xpath;
                        stype.name = stripTypeSuffix(type.name) + c_simpleContentTypeSuffx;
                        readRestriction(xpath + "/xsd:restriction", stype);
                        m_types.types[stype.name] = stype;

                        sc.type = stype.name;
                    } else {
                        // we simplify this case be creating a field for the value of the simpleContent
                        sc.type = document.textAttribute(xpath + "/xsd:restriction", "base");
                    }
                } else if (document.checkElement(xpath + "/xsd:extension")) {
                    // we simplify this case be creating a field for the value of the simpleContent
                    sc.type = document.textAttribute(xpath + "/xsd:extension", "base");
                }
                type.content = sc;
            }

            void readComplexContent(const std::string& xpath, ComplexType& type) {
                // <complexContent
                // id=ID
                // mixed=true|false
                // any attributes
                // >
                // (annotation?,(restriction|extension))
                // </complexContent>

                if (document.checkElement(xpath + "/xsd:restriction"))
                    throw NotImplementedException("XSD complexType complexContent restriction is not implemented");
                else if (document.checkElement(xpath + "/xsd:extension"))
                    readExtension(xpath + "/xsd:extension", type);
            }

            void readComplexTypeElementConfiguration(const std::string& xpath, ComplexType& type) {
                if (document.checkElement(xpath + "/xsd:all"))      type.content = readAll(xpath + "/xsd:all", stripTypeSuffix(type.name));
                else if (document.checkElement(xpath + "/xsd:sequence")) type.content = readSequence(xpath + "/xsd:sequence", stripTypeSuffix(type.name));
                else if (document.checkElement(xpath + "/xsd:choice"))   type.content = readChoice(xpath + "/xsd:choice", stripTypeSuffix(type.name));
                else if (document.checkElement(xpath + "/xsd:group"))    type.content = readGroup(xpath + "/xsd:group", stripTypeSuffix(type.name));
                else if (document.checkElement(xpath + "/xsd:any"))      type.content = readAny(xpath + "/xsd:any", stripTypeSuffix(type.name));

                if (document.checkElement(xpath + "/xsd:complexContent")) readComplexContent(xpath + "/xsd:complexContent", type);
                else if (document.checkElement(xpath + "/xsd:simpleContent"))  readSimpleContent(xpath + "/xsd:simpleContent", type);
            }

            Attribute readAttribute(const std::string& xpath, const std::string& containingTypeName) {
                Attribute att;
                att.xpath = xpath;
                att.name = document.textAttribute(xpath, "name");

                if (document.checkAttribute(xpath, "type"))
                    // referencing other type
                    att.type = document.textAttribute(xpath, "type");
                else
                    // type defined inline
                    att.type = readInlineType(xpath, containingTypeName + "_" + att.name);

                if (document.checkAttribute(xpath, "use")) {
                    const auto use = document.textAttribute(xpath, "use");
                    if (use == "optional")
                        att.optional = true;
                    else if (use == "required")
                        att.optional = false;
                    else
                        throw std::runtime_error("Invalid value for optional attribute at xpath: " + xpath);
                } else {
                    att.optional = true;
                }

                if (document.checkAttribute(xpath, "default"))
                    att.defaultValue = document.textAttribute(xpath, "default");
                if (document.checkAttribute(xpath, "fixed"))
                    att.fixed = document.textAttribute(xpath, "fixed");

                return att;
            }

            std::string readComplexType(const std::string& xpath, const std::string& nameHint = "") {
                // <complexType
                // id=ID
                // name=NCName
                // abstract=true|false
                // mixed=true|false
                // block=(#all|list of (extension|restriction))
                // final=(#all|list of (extension|restriction))
                // any attributes
                // >
                // (annotation?,(simpleContent|complexContent|((group|all|choice|sequence)?,((attribute|attributeGroup)*,anyAttribute?))))
                // </complexType>

                    // read or generate type name
                const std::string name = [&] {
                    if (document.checkAttribute(xpath, "name"))
                        return document.textAttribute(xpath, "name");
                    else
                        return generateUniqueTypeName(nameHint);
                }();

                if (m_types.types.find(name) != std::end(m_types.types))
                    throw std::runtime_error("Type with name " + name + " already exists");

                ComplexType type;
                type.xpath = xpath;
                type.name = name;

                if (document.checkAttribute(xpath, "id"))
                    throw NotImplementedException("XSD complextype id is not implemented");
                if (document.checkAttribute(xpath, "abstract"))
                    throw NotImplementedException("XSD complextype abstract is not implemented");
                if (document.checkAttribute(xpath, "mixed"))
                    throw NotImplementedException("XSD complextype mixed is not implemented");
                if (document.checkAttribute(xpath, "block"))
                    throw NotImplementedException("XSD complextype block is not implemented");
                if (document.checkAttribute(xpath, "final"))
                    throw NotImplementedException("XSD complextype final is not implemented");

                // read element configuration
                readComplexTypeElementConfiguration(xpath, type);

                // read attributes
                for (auto path : {
                    xpath,
                    xpath + "/xsd:complexContent/xsd:restriction",
                    xpath + "/xsd:complexContent/xsd:extension",
                    xpath + "/xsd:simpleContent/xsd:restriction",
                    xpath + "/xsd:simpleContent/xsd:extension",
                }) {
                    if (document.checkElement(path)) {
                        document.forEachChild(path + "/xsd:attribute", [&](const std::string& xpath) {
                            type.attributes.push_back(readAttribute(xpath, name));
                        });
                        if (document.checkElement(xpath + "/xsd:attributeGroup")) {
                            throw NotImplementedException("XSD complexType attributeGroup is not implemented");
                        }
                    }
                }

                // try to inline simple contents
                if (type.attributes.size() == 0 && type.base.empty() && type.content.is<SimpleContent>()) {
                    // this is just an empty type with a simple content, just use the type generated for the simple content
                    const auto& sc = type.content.as<SimpleContent>();
                    auto name = sc.type;

                    // if the inner typ is an enum, replace outer type, otherwise we assume it is a primitive type
                    auto& resolvedType = m_types.types[name];
                    if (resolvedType.is<SimpleType>() && resolvedType.as<SimpleType>().restrictionValues.size() > 0) {

                        // move simple content type out of type map
                        auto v = std::move(resolvedType);
                        m_types.types.erase(name);

                        // strip simple content suffix, if exists
                        const auto it = name.rfind(c_simpleContentTypeSuffx);
                        if (it == name.npos)
                            throw std::logic_error("Expected type of simple content type to have " + c_simpleContentTypeSuffx + " suffix");

                        // rename simple content type to outer type
                        assert(v.is<SimpleType>());
                        v.as<SimpleType>().name = name;

                        // readd it
                        m_types.types[name] = v;
                    }

                    return name;
                }

                // add
                m_types.types[name] = type;

                return name;
            }

            void readRestriction(const std::string& xpath, SimpleType& type) {
                // <restriction
                // id = ID
                // base = QName
                // any attributes
                // >
                // Content for simpleType:
                // (annotation?, (simpleType?, (minExclusive | minInclusive | maxExclusive | maxInclusive | totalDigits | fractionDigits | length | minLength | maxLength | enumeration | whiteSpace | pattern)*))
                // Content for simpleContent :
                // (annotation?, (simpleType?, (minExclusive | minInclusive | maxExclusive | maxInclusive | totalDigits | fractionDigits | length | minLength | maxLength | enumeration | whiteSpace | pattern)*)? , ((attribute | attributeGroup)*, anyAttribute ? ))
                // Content for complexContent:
                // (annotation?, (group | all | choice | sequence)?, ((attribute | attributeGroup)*, anyAttribute?))
                // </restriction>

                type.base = document.textAttribute(xpath, "base");

                document.forEachChild(xpath + "/xsd:enumeration", [&](const std::string& expath) {
                    const auto enumValue = document.textAttribute(expath, "value");
                    type.restrictionValues.push_back(enumValue);
                });

                auto writeError = [&](const char* element) {
                    std::cerr << "XSD restriction " << element << " is not implemented. No restrictions will be checked by generated code" << std::endl;
                };

                if (document.checkElement(xpath + "/xsd:simpleType"    )) throw NotImplementedException("XSD restriction simpleType is not implemented");
                if (document.checkElement(xpath + "/xsd:minExclusive"  )) writeError("minExclusive"  );
                if (document.checkElement(xpath + "/xsd:minInclusive"  )) writeError("minInclusive"  );
                if (document.checkElement(xpath + "/xsd:maxExclusive"  )) writeError("maxExclusive"  );
                if (document.checkElement(xpath + "/xsd:maxInclusive"  )) writeError("maxInclusive"  );
                if (document.checkElement(xpath + "/xsd:totalDigits"   )) writeError("totalDigits"   );
                if (document.checkElement(xpath + "/xsd:fractionDigits")) writeError("fractionDigits");
                if (document.checkElement(xpath + "/xsd:length"        )) writeError("length"        );
                if (document.checkElement(xpath + "/xsd:minLength"     )) writeError("minLength"     );
                if (document.checkElement(xpath + "/xsd:maxLength"     )) writeError("maxLength"     );
                if (document.checkElement(xpath + "/xsd:whiteSpace"    )) writeError("whiteSpace"    );
                if (document.checkElement(xpath + "/xsd:pattern"       )) writeError("pattern"       );
            }

            std::string readSimpleType(const std::string& xpath, const std::string& nameHint = "") {
                // <simpleType
                // id=ID
                // name=NCName
                // any attributes
                // >
                // (annotation?,(restriction|list|union))
                // </simpleType>

                // read or generate type name
                const std::string name = [&] {
                    if (document.checkAttribute(xpath, "name"))
                        return document.textAttribute(xpath, "name");
                    else {
                        return generateUniqueTypeName(nameHint);
                    }
                }();

                if (m_types.types.find(name) != std::end(m_types.types))
                    throw std::runtime_error("Type with name " + name + " already exists");

                SimpleType type;
                type.xpath = xpath;
                type.name = name;

                if (document.checkAttribute(xpath, "id"))
                    throw NotImplementedException("XSD complextype id is not implemented");

                     if (document.checkElement(xpath + "/xsd:restriction")) readRestriction(xpath + "/xsd:restriction", type);
                else if (document.checkElement(xpath + "/xsd:list"       )) throw NotImplementedException("XSD simpleType list is not implemented");
                else if (document.checkElement(xpath + "/xsd:union"      )) throw NotImplementedException("XSD simpleType union is not implemented");

                // add only simple types with restrictions (will become enums), otherwise just return underlying type
                if (type.restrictionValues.size() > 0) {
                    m_types.types[name] = type;
                    return name;
                } else
                    return type.base;
            }

            std::string readInlineType(const std::string& xpath, const std::string& nameHint) {
                     if (document.checkElement(xpath + "/xsd:complexType")) return readComplexType(xpath + "/xsd:complexType", nameHint);
                else if (document.checkElement(xpath + "/xsd:simpleType" )) return readSimpleType (xpath + "/xsd:simpleType",  nameHint);
                else throw std::runtime_error("Unexpected type or no type at xpath: " + xpath);
            }

            Element readElement(const std::string& xpath, const std::string& containingTypeName = "") {
                Element element;
                element.xpath = xpath;
                element.name = document.textAttribute(xpath, "name");

                // minOccurs
                if (!document.checkAttribute(xpath, "minOccurs"))
                    element.minOccurs = 1;
                else {
                    const auto minOccurs = document.textAttribute(xpath, "minOccurs");
                    element.minOccurs = std::stoi(minOccurs);
                }

                // maxOccurs
                if (!document.checkAttribute(xpath, "maxOccurs"))
                    element.maxOccurs = 1;
                else {
                    const auto maxOccurs = document.textAttribute(xpath, "maxOccurs");
                    if (maxOccurs == "unbounded")
                        element.maxOccurs = std::numeric_limits<decltype(element.maxOccurs)>::max();
                    else
                        element.maxOccurs = std::stoi(maxOccurs);
                }

                if (document.checkAttribute(xpath, "type"))
                    // referencing other type
                    element.type = document.textAttribute(xpath, "type");
                else
                    element.type = readInlineType(xpath, containingTypeName.empty() ? element.name : containingTypeName + "_" + element.name);

                assert(!element.type.empty());

                return element;
            }

            std::string generateUniqueTypeName(const std::string& newNameSuggestion) {
                auto toString = [](unsigned int id) {
                    if (id == 0)
                        return std::string();
                    else
                        return "_" + std::to_string(id);
                };

                unsigned int id = 0;
                while (m_types.types.find(newNameSuggestion + "Type" + toString(id)) != std::end(m_types.types))
                    id++;
                const auto n = newNameSuggestion + "Type" + toString(id);
                return n;
            }

            tixihelper::TixiDocument document;
            SchemaTypes m_types;
        };

        namespace {
            std::string stripSuffix(std::string name, const std::string& suffix) {
                const auto& s = suffix.size();
                if (name.size() > s && name.compare(name.size() - s, s, suffix) == 0)
                    name.erase(std::end(name) - s, std::end(name));
                return name;
            }
        }

        auto parseSchema(const std::string & cpacsLocation) -> SchemaTypes {
            SchemaParser schema(cpacsLocation);
            return std::move(schema.m_types);
        }

        auto stripTypeSuffix(const std::string& name) -> std::string {
            return stripSuffix(name, "Type");
        }

        auto stripSimpleContentSuffix(const std::string& name) -> std::string {
            return stripSuffix(name, "_SimpleContent");
        }
    }
}

#include "TypeSystem.h"

#include <boost/algorithm/string.hpp>

#include <iostream>
#include <cctype>
#include <fstream>

#include "NotImplementedException.h"
#include "Tables.h"

namespace tigl {
    namespace {
        auto makeClassName(std::string name) -> std::string {
            if (!name.empty()) {
                // capitalize first letter
                name[0] = std::toupper(name[0]);

                // strip Type suffix if exists
                name = stripTypeSuffix(name);

                // prefix CPACS
                name = "CPACS" + name;
            }

            return name;
        }

        auto resolveType(const SchemaTypes& types, const std::string& name, const Tables& tables) -> std::string {
            // search simple and complex types
            const auto cit = types.find(name);
            if (cit != std::end(types)) {
                const auto p = tables.m_typeSubstitutions.find(name);
                if (p)
                    return *p;
                else
                    return makeClassName(name);
            }

            // search predefined xml schema types and replace them
            const auto p = tables.m_typeSubstitutions.find(name);
            if (p)
                return *p;

            throw std::runtime_error("Unknown type: " + name);
        }

        auto buildFieldListAndChoiceSets(const SchemaTypes& types, const ComplexType& type, const Tables& tables) -> std::tuple<std::vector<Field>, std::vector<std::vector<unsigned int>>> {
            std::vector<Field> members;
            std::vector<std::vector<unsigned int>> choiceSets = { {} };

            // attributes
            for (const auto& a : type.attributes) {
                Field m;
                m.originXPath = a.xpath;
                m.cpacsName = a.name;
                m.typeName = resolveType(types, a.type, tables);
                m.xmlType = XMLConstruct::Attribute;
                if (a.optional)
                    m.cardinality = Cardinality::Optional;
                else
                    m.cardinality = Cardinality::Mandatory;
                members.push_back(m);
            }

            // elements
            struct ContentVisitor : public boost::static_visitor<> {
                ContentVisitor(const SchemaTypes& types, std::vector<Field>& members, std::vector<std::vector<unsigned int>>& choiceSets, unsigned int attributeCount, const Tables& tables)
                    : types(types), members(members), choiceSets(choiceSets), attributeCount(attributeCount), tables(tables) {}

                void operator()(const Element& e) const {
                    Field m;
                    m.originXPath = e.xpath;
                    m.cpacsName = e.name;
                    m.typeName = resolveType(types, e.type, tables);
                    m.xmlType = XMLConstruct::Element;
                    if (e.minOccurs == 0 && e.maxOccurs == 1)
                        m.cardinality = Cardinality::Optional;
                    else if (e.minOccurs == 1 && e.maxOccurs == 1)
                        m.cardinality = Cardinality::Mandatory;
                    else if (e.minOccurs >= 0 && e.maxOccurs > 1)
                        m.cardinality = Cardinality::Vector;
                    else if (e.minOccurs == 0 && e.maxOccurs == 0) {
                        std::cerr << "Warning: Element " + e.name + " with type " + e.type + " was omitted as minOccurs and maxOccurs are both zero" << std::endl;
                        return; // skip this type
                    } else
                        throw std::runtime_error("Invalid cardinalities, min: " + std::to_string(e.minOccurs) + ", max: " + std::to_string(e.maxOccurs));
                    for (auto& cs : choiceSets)
                        cs.push_back(static_cast<unsigned int>(members.size()));
                    members.push_back(m);
                }

                void operator()(const Choice& c) const {
                    unsigned int choiceIndex = 1;
                    const auto prevChoiceSets = std::move(choiceSets);
                    choiceSets.clear();
                    const auto countBefore = members.size();
                    for (const auto& v : c.elements) {
                        // collect members of one choice
                        std::vector<Field> choiceMembers;
                        std::vector<std::vector<unsigned int>> choiceChoiceSets = { {} };
                        v.visit(ContentVisitor(types, choiceMembers, choiceChoiceSets, attributeCount, tables));

                        // make all optional
                        for (auto& f : choiceMembers)
                            if (f.cardinality == Cardinality::Mandatory)
                                f.cardinality = Cardinality::Optional;

                        // give custom names
                        for (auto& f : choiceMembers)
                            f.customName = f.cpacsName + "_choice" + std::to_string(choiceIndex);

                        // copy to output
                        const auto choiceCountBefore = members.size();
                        std::copy(std::begin(choiceMembers), std::end(choiceMembers), std::back_inserter(members));

                        // for each previous choice set
                        for (const auto& pcs : prevChoiceSets)
                            // take each new choice set
                            for (auto& cs : choiceChoiceSets) {
                                // offset new and combien with prev
                                auto ncs = pcs;
                                std::transform(std::begin(cs), std::end(cs), std::back_inserter(ncs), [&](int i) { return i + static_cast<unsigned int>(choiceCountBefore); });
                                // release it
                                choiceSets.push_back(std::move(ncs));
                            }

                        choiceIndex++;
                    }

                    // consistency check, two types with the same name but different types or cardinality are problematic
                    for (std::size_t i = countBefore; i < members.size(); i++) {
                        const auto& f1 = members[i];
                        for (std::size_t j = i + 1; j < members.size(); j++) {
                            const auto& f2 = members[j];
                            if (f1.cpacsName == f2.cpacsName && (f1.cardinality != f2.cardinality || f1.typeName != f2.typeName)) {
                                std::cerr << "Elements with same name but different cardinality or type inside choice" << std::endl;
                                for (const auto& f : { f1, f2 })
                                    std::cerr << f.cpacsName << " " << toString(f.cardinality) << " " << f.typeName << std::endl;
                            }
                        }
                    }
                }

                void operator()(const Sequence& s) const {
                    for (const auto& v : s.elements)
                        v.visit(ContentVisitor(types, members, choiceSets, attributeCount, tables));
                }

                void operator()(const All& a) const {
                    for (const auto& e : a.elements)
                        operator()(e);
                }

                void operator()(const Any& a) const {
                    throw NotImplementedException("Generating fields for any is not implemented");
                }

                void operator()(const Group& g) const {
                    throw NotImplementedException("Generating fields for group is not implemented");
                }

                void operator()(const SimpleContent& g) const {
                    Field m;
                    m.originXPath = g.xpath;
                    m.cpacsName = "";
                    m.customName = "simpleContent";
                    m.cardinality = Cardinality::Mandatory;
                    m.typeName = resolveType(types, g.type, tables);
                    m.xmlType = XMLConstruct::SimpleContent;
                    for (auto& cs : choiceSets)
                        cs.push_back(static_cast<unsigned int>(members.size()));
                    members.push_back(m);
                }

            private:
                const SchemaTypes& types;
                std::vector<Field>& members;
                std::vector<std::vector<unsigned int>>& choiceSets;
                const unsigned int attributeCount;
                const Tables& tables;
            };

            type.content.visit(ContentVisitor(types, members, choiceSets, members.size(), tables));

            for (const auto& cs : choiceSets)
                assert(std::is_sorted(std::begin(cs), std::end(cs)));

            return std::make_tuple(members, choiceSets);
        }
    }

    class TypeSystemBuilder {
    public:
        TypeSystemBuilder(SchemaTypes types, const Tables& tables)
            : m_types(std::move(types)), tables(tables) {}

        void build() {
            for (const auto& p : m_types) {
                const auto& type = p.second;

                struct TypeVisitor {
                    TypeVisitor(const SchemaTypes& types, TypeSystemBuilder& typeSystem, const Tables& tables)
                        : types(types), typeSystem(typeSystem), tables(tables) {}

                    void operator()(const ComplexType& type) {
                        Class c;
                        c.originXPath = type.xpath;
                        c.name = makeClassName(type.name);
                        std::tie(c.fields, c.choiceSets) = buildFieldListAndChoiceSets(types, type, tables);

                        if (c.choiceSets.size() > 1) {
                            // find unique elements for each choice
                            for (const auto& cs : c.choiceSets) {
                                std::vector<unsigned int> uniqueIndices;
                                for (auto i : cs) {
                                    // search other choice sets for this index
                                    const auto found = [&] {
                                        for (const auto& cs2 : c.choiceSets)
                                            if (&cs2 != &cs)
                                                for (auto j : cs2)
                                                    if (j == i)
                                                        return true;
                                        return false;
                                    }();
                                    if (!found)
                                        uniqueIndices.push_back(i);
                                }
                                if (uniqueIndices.empty()) {
                                    std::cout << "Warning: From the set of choices" << std::endl;
                                    auto counter = 0;
                                    for (const auto& cs : c.choiceSets) {
                                        std::vector<std::string> elementList;
                                        elementList.reserve(cs.size());
                                        std::transform(std::begin(cs), std::end(cs), std::back_inserter(elementList), [&](unsigned int i) { return c.fields[i].cpacsName; });
                                        std::cout << "\t" << counter++ << ": " << boost::join(elementList, ", ") << std::endl;
                                    }
                                    std::cout << "the choice " << (&cs - &c.choiceSets[0]) << " cannot be uniquely identified" << std::endl;
                                }
                                c.choiceUniqueFields.push_back(std::move(uniqueIndices));
                            }
                        }

                        if (!type.base.empty()) {
                            c.base = resolveType(types, type.base, tables);

                            // make base a field if fundamental type
                            if (tables.m_fundamentalTypes.contains(c.base)) {
                                std::cout << "\tWarning: Type " << type.name << " has base class " << c.base << " which is a fundamental type. Generated field 'base' instead" << std::endl;

                                Field f;
                                f.cpacsName = "";
                                f.customName = "base";
                                f.cardinality = Cardinality::Mandatory;
                                f.typeName = c.base;
                                f.xmlType = XMLConstruct::FundamentalTypeBase;

                                c.fields.insert(std::begin(c.fields), f);
                                c.base.clear();
                            }
                        }

                        typeSystem.m_classes[c.name] = c;
                    }

                    void operator()(const SimpleType& type) {
                        if (type.restrictionValues.size() > 0) {
                            // create enum
                            Enum e;
                            e.originXPath = type.xpath;
                            e.name = makeClassName(type.name);
                            for (const auto& v : type.restrictionValues)
                                e.values.push_back(EnumValue(v));
                            typeSystem.m_enums[e.name] = e;
                        } else
                            throw NotImplementedException("Simple times which are not m_enums are not implemented");
                    }

                private:
                    const SchemaTypes& types;
                    TypeSystemBuilder& typeSystem;
                    const Tables& tables;
                };

                type.visit(TypeVisitor(m_types, *this, tables));
            };
        }

        // TODO: replace by lambda when C++14 is available
        struct SortAndUnique {
            template <typename T>
            void operator()(std::vector<T*>& con) {
                std::sort(std::begin(con), std::end(con), [](const T* a, const T* b) {
                    return a->name < b->name;
                });
                con.erase(std::unique(std::begin(con), std::end(con), [](const T* a, const T* b) {
                    return a->name == b->name;
                }), std::end(con));
            }
        };

        void buildDependencies() {
            std::cout << "Building dependencies" << std::endl;

            for (auto& p : m_classes) {
                auto& c = p.second;
                if (c.pruned)
                    continue;

                // base
                if (!c.base.empty()) {
                    const auto it = m_classes.find(c.base);
                    if (it != std::end(m_classes)) {
                        if (!it->second.pruned) {
                            c.deps.bases.push_back(&it->second);
                            it->second.deps.deriveds.push_back(&c);
                        }
                    } else
                        // this exception should be prevented by earlier code
                        throw std::runtime_error("Class " + c.name + " has non-class base: " + c.base);
                }

                // fields
                for (auto& f : c.fields) {
                    const auto eit = m_enums.find(f.typeName);
                    if (eit != std::end(m_enums)) {
                        if (!eit->second.pruned) {
                            c.deps.enumChildren.push_back(&eit->second);
                            eit->second.deps.parents.push_back(&c);
                        }
                    } else {
                        const auto cit = m_classes.find(f.typeName);
                        if (cit != std::end(m_classes)) {
                            if (!cit->second.pruned) {
                                c.deps.children.push_back(&cit->second);
                                cit->second.deps.parents.push_back(&c);
                            }
                        }
                    }
                }
            }

            SortAndUnique sortAndUnique;

            // sort and unique
            for (auto& p : m_classes) {
                auto& c = p.second;
                sortAndUnique(c.deps.bases);
                sortAndUnique(c.deps.children);
                sortAndUnique(c.deps.deriveds);
                sortAndUnique(c.deps.enumChildren);
                sortAndUnique(c.deps.parents);
            }

            for (auto& p : m_enums) {
                auto& e = p.second;
                sortAndUnique(e.deps.parents);
            }
        }

        void collapseEnums() {
            std::cout << "Collapsing enums" << std::endl;

            // convert enum unordered_map to vector for easier processing
            std::vector<Enum> enumVec;
            enumVec.reserve(m_enums.size());
            for (const auto& p : m_enums)
                enumVec.push_back(p.second);

            std::unordered_map<std::string, std::string> replacedEnums;

            for (std::size_t i = 0; i < enumVec.size(); i++) {
                auto& e1 = enumVec[i];
                for (std::size_t j = i + 1; j < enumVec.size(); j++) {
                    auto& e2 = enumVec[j];

                    auto stripNumber = [](std::string name) {
                        // handle inline enum types
                        // generated names are of the form: <containing type name>_<element name>[_SimpleContent][_<counter>]

                        // remove optional digits and underscore at the end
                        while (!name.empty() && std::isdigit(name.back()))
                            name.pop_back();
                        if (name.back() == '_')
                            name.erase(name.length() - 1);
                    
                        // remove _SimpleContent
                        name = stripSimpleContentSuffix(name);

                        // if type contains an underscore, remove preceding part
                        const auto& pos = name.find_last_of('_');
                        if (pos != std::string::npos)
                            name.erase(0, pos + 1);

                        // capitalize first letter
                        name[0] = std::toupper(name[0]);

                        // strip Type suffix if exists
                        name = stripTypeSuffix(name);

                        // prefix CPACS if not exists
                        if (name.compare(0, 5, "CPACS") != 0)
                            name = "CPACS" + name;

                        return name;
                    };

                    if (e1.values.size() == e2.values.size()) {
                        // strip name decorators
                        const auto& e1StrippedName = stripNumber(e1.name);
                        const auto& e2StrippedName = stripNumber(e2.name);
                        if (e1StrippedName == e2StrippedName && std::equal(std::begin(e1.values), std::end(e1.values), std::begin(e2.values))) {
                            // choose new name
                            const auto newName = [&] {
                                // if the stripped name is not already taken, use it. Otherwise, take the shorter of the two enum names
                                if (m_classes.count(e1StrippedName) == 0 && m_enums.count(e1StrippedName) == 0)
                                    return e1StrippedName;
                                else
                                    return std::min(e1.name, e2.name);
                            }();

                            // register replacements
                            if (e1.name != newName) replacedEnums[e1.name] = newName;
                            if (e2.name != newName) replacedEnums[e2.name] = newName;

                            std::cout << "\t" << e1.name << " and " << e2.name << " to " << newName << std::endl;

                            // remove e2 and rename e1
                            enumVec.erase(std::begin(enumVec) + j);
                            e1.name = newName;
                        }
                    }
                }
            }

            // fill enum back again from vector
            m_enums.clear();
            for (auto& e : enumVec)
                m_enums[e.name] = std::move(e);

            // replace enum names
            for (auto& p : m_classes) {
                auto& c = p.second;
                for (auto& f : c.fields) {
                    const auto& rit = replacedEnums.find(f.typeName);
                    if (rit != std::end(replacedEnums))
                        f.typeName = rit->second;
                }
            }
        }

        void prefixClashedEnumValues() {
            std::unordered_map<std::string, std::vector<Enum*>> valueToEnum;

            for (auto& p : m_enums) {
                auto& e = p.second;
                for (auto& v : e.values) {
                    auto& otherEnums = valueToEnum[v.name()];
                    if (otherEnums.size() == 1) {
                        // we are adding the same enum value for the second time
                        // prefix other enum's value
                        auto& otherEnum = *otherEnums[0];
                        const auto it = std::find_if(std::begin(otherEnum.values), std::end(otherEnum.values), [&](const EnumValue& ov) {
                            return ov.name() == v.name();
                        });
                        if (it == std::end(otherEnum.values))
                            throw std::logic_error("Enum value resolves to an enum which does not have the value");
                        it->customName = otherEnum.name + "_" + it->cpacsName;
                    }
                    if (otherEnums.size() > 1) {
                        // we are adding an already added value, prefix myself
                        v.customName = e.name + "_" + v.cpacsName;
                    }

                    otherEnums.push_back(&e);
                }
            }

            std::cout << "Prefixed the following enum values:" << std::endl;
            for (auto& p : valueToEnum) {
                const auto& otherEnums = p.second;
                if (otherEnums.size() > 1) {
                    std::cout << '\t' << p.first << std::endl;
                    for (const auto& e : otherEnums)
                        std::cout << "\t\t" << e->name << std::endl;
                }
            }
        }

        auto checkAndPrintNode(const std::string& name, const Table& pruneList, unsigned int level) {
            if (pruneList.contains(name)) {
                std::cout << std::string(level, '\t') <<  "pruning " << name << std::endl;
                return true;
            } else {
                std::cout << std::string(level, '\t') << "including " << name << std::endl;
                return false;
            }
        }

        void includeNode(Enum& e, const Table& pruneList, unsigned int level) {
            // if this enum is already marked, return
            if (e.pruned == false)
                return;

            // if this enum is on the prune list, just leave it
            if (checkAndPrintNode(e.name, pruneList, level))
                return;

            // enum is not pruned, mark it
            e.pruned = false;
        }

        void includeNode(Class& cls, const Table& pruneList, unsigned int level) {
            // if this class is already marked, return
            if (cls.pruned == false)
                return;


            // if this class is on the prune list, just leave it and all its sub element types pruned
            if (checkAndPrintNode(cls.name, pruneList, level))
                return;

            // class is not pruned, mark it
            cls.pruned = false;

            auto& deps = cls.deps;

            // try to include all bases
            for (auto& b : deps.bases)
                includeNode(*b, pruneList, level + 1);

            // try to include all field classes
            for (auto& c : deps.children)
                includeNode(*c, pruneList, level + 1);

            // try to include all field m_enums
            for (auto& e : deps.enumChildren)
                includeNode(*e, pruneList, level + 1);
        }

        void runPruneList() {
            std::cout << "Running prune list" << std::endl;

            // mark all types as pruned
            for (auto& p : m_classes)
                p.second.pruned = true;
            for (auto& p : m_enums)
                p.second.pruned = true;

            // find root type
            const auto rootElementTypeName = std::string("CPACSCpacs");
            const auto it = m_classes.find(rootElementTypeName);
            if (it == std::end(m_classes))
                throw std::runtime_error("Could not find root element. Expected: " + rootElementTypeName);
            auto& root = it->second;

            includeNode(root, tables.m_pruneList, 0);

            std::cout << "The following types have been pruned:" << std::endl;
            std::vector<std::string> prunedTypeNames;
            for (const auto& p : m_classes)
                if(p.second.pruned)
                    prunedTypeNames.push_back("Class: " + p.second.name);
            for (const auto& p : m_enums)
                if (p.second.pruned)
                    prunedTypeNames.push_back("Enum: " + p.second.name);
            std::sort(std::begin(prunedTypeNames), std::end(prunedTypeNames));
            for(const auto& name : prunedTypeNames)
                std::cout << "\t" << name << std::endl;

            auto isPruned = [&](const std::string& name) {
                const auto& cit = m_classes.find(name);
                if (cit != std::end(m_classes) && cit->second.pruned)
                    return true;
                const auto& eit = m_enums.find(name);
                if (eit != std::end(m_enums) && eit->second.pruned)
                    return true;
                return false;
            };

            // remove pruned classes and enums from class fields and bases
            for (auto& c : m_classes) {
                auto& fields = c.second.fields;
                fields.erase(std::remove_if(std::begin(fields), std::end(fields), [&](const Field& f) {
                    return isPruned(f.typeName);
                }), std::end(fields));

                auto& baseName = c.second.base;
                if (isPruned(baseName))
                    baseName.clear();
            }

            // clear and rebuild dependencies
            for (auto& c : m_classes)
                c.second.deps = ClassDependencies{};
            for (auto& e : m_enums)
                e.second.deps = EnumDependencies{};
            buildDependencies();
        }

        const Tables& tables;
        SchemaTypes m_types;
        std::unordered_map<std::string, Class> m_classes;
        std::unordered_map<std::string, Enum> m_enums;
    };

    auto buildTypeSystem(SchemaTypes types, const Tables& tables) -> TypeSystem {
        TypeSystemBuilder builder(std::move(types), tables);
        builder.build();
        builder.collapseEnums();
        builder.prefixClashedEnumValues();
        builder.buildDependencies();
        builder.runPruneList();
        return {
            std::move(builder.m_classes),
            std::move(builder.m_enums)
        };
    }

    void writeGraphVisFile(const TypeSystem& ts, const std::string& typeSystemGraphVisFile) {
        std::cout << "Writing type system graph vis file to " << typeSystemGraphVisFile << std::endl;
        std::ofstream f{typeSystemGraphVisFile};
        if (!f)
            throw std::runtime_error("Failed to open file " + typeSystemGraphVisFile + " for writing");
        f << "digraph typesystem {\n";
        for (const auto& p : ts.classes) {
            const auto& c = p.second;
            if (c.pruned)
                continue;
            for (const auto& b : c.deps.bases)
                f << "\t" << c.name << " -> " << b->name << ";\n";
            for (const auto& ch : c.deps.children)
                f << "\t" << c.name << " -> " << ch->name << ";\n";
            for (const auto& e : c.deps.enumChildren)
                f << "\t" << c.name << " -> " << e->name << ";\n";
        }
        // enums have no further dependencies
        f << "}\n";
    }
}

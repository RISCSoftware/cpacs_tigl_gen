#include <boost/algorithm/string.hpp>
#include <boost/utility/in_place_factory.hpp>

#include <utility>
#include <set>
#include <vector>
#include <cctype>
#include <algorithm>
#include <sstream>
#include <iomanip>

#include "SchemaParser.h"
#include "Tables.h"
#include "TypeSystem.h"
#include "NotImplementedException.h"
#include "IndentingStreamWrapper.h"
#include "Filesystem.h"
#include "CodeGen.h"

namespace tigl {
    // some options
    namespace {
        const auto c_generateDefaultCtorsForParentPointerTypes = false;
        const auto c_generateCaseSensitiveStringToEnumConversion = false; // true would be more strict, but some test data has troubles with this
        const auto c_generateTryCatchAroundOptionalClassReads = true;
        const auto c_generateCpp11ScopedEnums = false;

        const auto tixiHelperNamespace = "tixi";
        const auto c_uidMgrName = std::string("CTiglUIDManager");
        const auto c_unboundedConstantName = "tixi::xsdUnbounded";
        const auto c_uidRefType = std::string("stringUIDBaseType");
    }

    namespace {
        auto customReplacedType(const std::string& type, const Tables& tables) -> const std::string& {
            const auto p = tables.m_customTypes.find(type);
            return p ? *p : type;
        }

        auto capitalizeFirstLetter(std::string str) -> std::string {
            if (str.empty())
                return str;

            str[0] = std::toupper(str[0]);

            return str;
        }

        auto enumToStringFunc(const Enum& e, const Tables& tables) -> std::string {
            return customReplacedType(e.name, tables) + "ToString";
        }

        auto stringToEnumFunc(const Enum& e, const Tables& tables) -> std::string {
            return "stringTo" + capitalizeFirstLetter(customReplacedType(e.name, tables));
        }

        auto uidReferenceFields(const Class& c) {
            std::vector<Field> uidRefFields;
            for (const auto& f : c.fields)
                if (f.xmlTypeName == c_uidRefType)
                    uidRefFields.push_back(f);
            return uidRefFields;
        }

        auto hasUidField(const Class& c) {
            for (const auto& f : c.fields)
                if (f.name() == "uID")
                    return true;
            return false;
        }

        auto hasUidRefField(const Class& c) {
            return !uidReferenceFields(c).empty();
        }

        auto hasMandatoryUidField(const Class& c) {
            for (const auto& f : c.fields)
                if (f.name() == "uID" && f.cardinality() == Cardinality::Mandatory)
                    return true;
            return false;
        }

        auto hasInheritedUidField(const Class& c) -> bool {
            for (const auto& cc : c.deps.bases)
                if (hasUidField(*cc) || hasInheritedUidField(*cc))
                    return true;
            return false;
        }

        // TODO: this call recurses down the tree, if this gets too slow, we can traverse the tree once and mark all classes
        auto selfOrBaseOrAnyChildOfSelfRequiresUidMgr(const Class& c) {
            // UidMgr required in case class has a uid field or uid reference field
            if (hasUidField(c) || hasUidRefField(c))
                return true;
            for (const auto& cc : c.deps.bases)
                if (selfOrBaseOrAnyChildOfSelfRequiresUidMgr(*cc))
                    return true;
            for (const auto& cc : c.deps.children)
                if (selfOrBaseOrAnyChildOfSelfRequiresUidMgr(*cc))
                    return true;
            return false;
        }

        auto requiresUidManager(const Class& c) {
            return selfOrBaseOrAnyChildOfSelfRequiresUidMgr(c);
        }

        auto requiresUidManagerField(const Class& c) {
            return requiresUidManager(c) && !hasInheritedUidField(c);
        }

        auto formatMaxOccurs(unsigned int maxOccurs) -> std::string {
            if (maxOccurs == tigl::xsd::unbounded)
                return c_unboundedConstantName;
            return std::to_string(maxOccurs);
        }

        auto elementNames(const Class& c) {
            std::vector<std::string> elemNames;
            for (const auto& f : c.fields)
                if (f.xmlType != XMLConstruct::Attribute)
                    elemNames.push_back(f.cpacsName);
            return elemNames;
        }

        bool uniqueChildElements(const Class& c) {
            const auto elemNames = elementNames(c);
            const std::set<std::string> uniqueElemNames(elemNames.begin(), elemNames.end());
            return uniqueElemNames.size() == elemNames.size();
        }
    }

    class CodeGen {
    public:
        CodeGen(TypeSystem types, std::string ns, const Tables& tables)
            : m_types(std::move(types)), m_namespace(std::move(ns)), m_tables(tables) {}

        void writeFiles(const std::string& outputLocation, Filesystem& fs) {
            for (const auto& p : m_types.classes) {
                const auto c = p.second;
                const auto hppFileName = outputLocation + "/" + c.name + ".h";
                const auto cppFileName = outputLocation + "/" + c.name + ".cpp";
                if (c.pruned) {
                    fs.removeIfExists(hppFileName);
                    fs.removeIfExists(cppFileName);
                    continue;
                }

                auto& hpp = fs.newFile(hppFileName);
                auto& cpp = fs.newFile(cppFileName);
                IndentingStreamWrapper hppStream(hpp.stream());
                IndentingStreamWrapper cppStream(cpp.stream());
                writeClass(hppStream, cppStream, c);
            }

            for (const auto& p : m_types.enums) {
                const auto e = p.second;
                const auto hppFileName = outputLocation + "/" + e.name + ".h";
                if (e.pruned) {
                    fs.removeIfExists(hppFileName);
                    continue;
                }

                auto& hpp = fs.newFile(hppFileName);
                IndentingStreamWrapper hppStream(hpp.stream());
                writeEnum(hppStream, e);
            }
        }

    private:
        struct Includes {
            std::vector<std::string> hppIncludes;
            std::vector<std::string> hppForwards;
            std::vector<std::string> hppCustomForwards;
            std::vector<std::string> cppIncludes;
        };

        TypeSystem m_types;
        std::string m_namespace;
        const Tables& m_tables;

        auto customReplacedType(const std::string& name) const -> std::string {
            return tigl::customReplacedType(name, m_tables);
        }

        auto customReplacedType(const Field& field) const -> std::string {
            return customReplacedType(field.typeName);
        }

        auto requiresParentPointer(const Class& c) const {
            return !c.deps.parents.empty() && (m_tables.m_parentPointers.contains(c.name) || c_allTypesGetParentPointer);
        }

        auto getterSetterType(const Field& field) const -> std::string {
            const auto typeName = customReplacedType(field);
            switch (field.cardinality()) {
                case Cardinality::Optional:
                    return "boost::optional<" + typeName + ">";
                case Cardinality::Mandatory:
                    return typeName;
                case Cardinality::Vector:
                {
                    if (m_types.classes.find(field.typeName) != std::end(m_types.classes))
                        return "std::vector<std::unique_ptr<" + typeName + ">>";
                    else
                        return "std::vector<" + typeName + ">";
                }
                default:
                    throw std::logic_error("Invalid cardinality");
            }
        }

        auto fieldType(const Field& field) const -> std::string {
            return getterSetterType(field);
        }

        auto vectorInnerType(const Field& field) const -> std::string {
            if (field.cardinality() != Cardinality::Vector)
                throw std::logic_error("Requested vector inner type for non-vector type");
            return customReplacedType(field);
        }

        void writeDocumentation(IndentingStreamWrapper& hpp, const std::string& documentation) const {
            if (!documentation.empty()) {
                std::vector<std::string> lines;
                boost::algorithm::split(lines, documentation, boost::is_any_of("\n"));
                for (const auto& line : lines)
                    hpp << "/// " << line;
            }
        }

        void writeFields(IndentingStreamWrapper& hpp, const std::vector<Field>& fields) const {
            std::size_t length = 0;
            for (const auto& f : fields)
                length = std::max(length, fieldType(f).length());

            const auto haveAnyDocumentation = std::any_of(begin(fields), end(fields), [](const Field& f) {
                return !f.documentation.empty();
            });
            for (const auto& f : fields) {
                writeDocumentation(hpp, f.documentation);
                hpp << std::left << std::setw(length) << fieldType(f) << " " << f.fieldName() << ";";

                if (haveAnyDocumentation || &f == &fields.back())
                    hpp << EmptyLine;
            }
        }

        void writeAccessorDeclarations(IndentingStreamWrapper& hpp, const std::vector<Field>& fields) const {
            for (const auto& f : fields) {
                hpp << "TIGL_EXPORT virtual const " << getterSetterType(f) << "& Get" << capitalizeFirstLetter(f.name()) << "() const;";
                // generate setter only for fundamental and enum types which are not vectors
                const bool isClassType = m_types.classes.find(f.typeName) != std::end(m_types.classes);
                if (!isClassType && f.cardinality() != Cardinality::Vector) {
                    hpp << "TIGL_EXPORT virtual void Set" << capitalizeFirstLetter(f.name()) << "(const " << getterSetterType(f) << "& value);";
                } //generate accessors for vector types
                else if( f.cardinality() == Cardinality::Vector && f.xmlTypeName !=c_uidRefType) {
                    hpp << "TIGL_EXPORT virtual " << getterSetterType(f) << "& Get" << capitalizeFirstLetter(f.name()) << "();";
                    hpp << "TIGL_EXPORT int Get" << capitalizeFirstLetter(f.cpacsName) << "Count() const;";
                    hpp << "TIGL_EXPORT " << vectorInnerType(f) << "& Get" << capitalizeFirstLetter(f.cpacsName) << "(int index) const;";
                } // generate special accessors for uid reference vectors
                else if (f.cardinality() == Cardinality::Vector && f.xmlTypeName == c_uidRefType) {
                    hpp << "TIGL_EXPORT virtual void AddTo" << capitalizeFirstLetter(f.name()) << "(const " << vectorInnerType(f) << "& value);";
                    hpp << "TIGL_EXPORT virtual bool RemoveFrom" << capitalizeFirstLetter(f.name()) << "(const " << vectorInnerType(f) << "& value);";
                }
                else
                    hpp << "TIGL_EXPORT virtual " << getterSetterType(f) << "& Get" << capitalizeFirstLetter(f.name()) << "();";
                hpp << EmptyLine;
            }
        }

        void writeAccessorImplementations(IndentingStreamWrapper& cpp, const std::string& className, const std::vector<Field>& fields) const {
            for (const auto& f : fields) {
                cpp << "const " << getterSetterType(f) << "& " << className << "::Get" << capitalizeFirstLetter(f.name()) << "() const";
                cpp << "{";
                {
                    Scope s(cpp);
                    cpp << "return " << f.fieldName() << ";";
                }
                cpp << "}";
                cpp << EmptyLine;

                // generate setter only for fundamental and enum types which are not vectors
                const bool isClassType = m_types.classes.find(f.typeName) != std::end(m_types.classes);
                if (!isClassType && f.cardinality() != Cardinality::Vector) {
                    auto writeUidRegistration = [&](bool isOptional) {
                        if (f.name() == "uID") {
                            cpp << "if (m_uidMgr && value != m_uID) {";
                            {
                                Scope s(cpp);
                                if (isOptional) {
                                    cpp << "if (!m_uID && value) {";
                                    {
                                        Scope s(cpp);
                                        cpp << "m_uidMgr->RegisterObject(*value, *this);";
                                    }
                                    cpp << "}";
                                    cpp << "else if (m_uID && !value) {";
                                    {
                                        Scope s(cpp);
                                        cpp << "m_uidMgr->TryUnregisterObject(*m_uID);";
                                    }
                                    cpp << "}";
                                    cpp << "else if (m_uID && value) {";
                                    {
                                        Scope s(cpp);
                                        cpp << "m_uidMgr->UpdateObjectUID(*m_uID, *value);";
                                    }
                                    cpp << "}";
                                }
                                else {
                                    cpp << "if (m_uID.empty()) {";
                                    {
                                        Scope s(cpp);
                                        cpp << "m_uidMgr->RegisterObject(value, *this);";
                                    }
                                    cpp << "}";
                                    cpp << "else {";
                                    {
                                        Scope s(cpp);
                                        cpp << "m_uidMgr->UpdateObjectUID(m_uID, value);";
                                    }
                                    cpp << "}";
                                }
                            }
                            cpp << "}";
                        }
                    };

                    auto writeUidReferenceRegistration = [&](bool isOptional) {
                        if (f.xmlTypeName == c_uidRefType) {
                            cpp << "if (m_uidMgr) {";
                            {
                                Scope s(cpp);
                                if (isOptional)
                                    cpp << "if (" << f.fieldName() << " && !" << f.fieldName() << "->empty()) m_uidMgr->TryUnregisterReference(*" << f.fieldName() << ", *this);";
                                else
                                    cpp << "if (!" << f.fieldName() << ".empty()) m_uidMgr->TryUnregisterReference(" << f.fieldName() << ", *this);";

                                if (isOptional)
                                    cpp << "if (value && !value->empty()) m_uidMgr->RegisterReference(*value, *this);";
                                else
                                    cpp << "if (!value.empty()) m_uidMgr->RegisterReference(value, *this);";
                            }
                            cpp << "}";
                        }
                    };

                    cpp << "void " << className << "::Set" << capitalizeFirstLetter(f.name()) << "(const " << getterSetterType(f) << "& value)";
                    cpp << "{";
                    {
                        Scope s(cpp);
                        const auto isOptional = f.cardinality() == Cardinality::Optional;
                        writeUidRegistration(isOptional);
                        writeUidReferenceRegistration(isOptional);
                        cpp << f.fieldName() << " = value;";
                    }
                    cpp << "}";
                }
                else if( f.cardinality() == Cardinality::Vector && f.xmlTypeName !=c_uidRefType) {  //Check if this is necessary, or if the code belongs below with the "else" block
                    //getter for classType
                    cpp << getterSetterType(f) << "& " << className << "::Get" << capitalizeFirstLetter(f.name()) << "()";
                    cpp << "{";
                    {
                        Scope s(cpp);
                        cpp << "return " << f.fieldName() << ";";
                    }
                    cpp << "}";
                    cpp << EmptyLine;

                    // Getter for element count in vector type;
                    cpp << "int " << className << "::Get" << capitalizeFirstLetter(f.cpacsName) << "Count() const";
                    cpp << "{";
                    {
                        Scope s(cpp);
                        cpp << f.fieldName() << ".size();";
                    }
                    cpp << "}";
                    cpp << EmptyLine;

                    // Getter for elements in vector type by index;
                    cpp << vectorInnerType(f) << "& " << className << "::Get" << capitalizeFirstLetter(f.cpacsName) << "(int index) const";
                    cpp << "{";
                    {
                        Scope s(cpp);
                        cpp << "index--;";
                        cpp << "if (index < 0 || index >= Get" << capitalizeFirstLetter(f.name()) << "Count()) {";
                        {
                            Scope s(cpp);
                            cpp << "throw CTiglError(\"Invalid indexs in " << getterSetterType(f) << "::Get" << capitalizeFirstLetter(f.cpacsName) << "\", TIGL_INDEX_ERROR);";
                        }
                         cpp << "}";
                         cpp << "return *" << f.fieldName() << "[index];";
                     }
                     cpp << "}";
                     cpp << EmptyLine;


                }// generate special accessors for uid reference vectors
                else if (f.cardinality() == Cardinality::Vector && f.xmlTypeName == c_uidRefType) {
                    cpp << "void " << className << "::AddTo" << capitalizeFirstLetter(f.name()) << "(const " << vectorInnerType(f) << "& value)";
                    cpp << "{";
                    {
                        Scope s(cpp);
                        cpp << "if (m_uidMgr) {";
                        {
                            Scope s(cpp);
                            cpp << "if (!value.empty()) m_uidMgr->RegisterReference(value, *this);";
                        }
                        cpp << "}";
                        cpp << f.fieldName() << ".push_back(value);";
                    }
                    cpp << "}";
                    cpp << EmptyLine;
                    cpp << "bool " << className << "::RemoveFrom" << capitalizeFirstLetter(f.name()) << "(const " << vectorInnerType(f) << "& value)";
                    cpp << "{";
                    {
                        Scope s(cpp);
                        cpp << "const auto it = std::find(" << f.fieldName() << ".begin(), " << f.fieldName() << ".end(), value);";
                        cpp << "if (it != " << f.fieldName() << ".end()) {";
                        {
                            Scope s(cpp);
                            cpp << "if (m_uidMgr && !it->empty()) {";
                            {
                                Scope s(cpp);
                                cpp << "m_uidMgr->TryUnregisterReference(*it, *this);";
                            }
                            cpp << "}";
                            cpp << f.fieldName() << ".erase(it);";
                            cpp << "return true;";
                        }
                        cpp << "}";
                        cpp << "return false;";
                    }
                    cpp << "}";
                }
                else {
                    cpp << getterSetterType(f) << "& " << className << "::Get" << capitalizeFirstLetter(f.name()) << "()";
                    cpp << "{";
                    {
                        Scope s(cpp);
                        cpp << "return " << f.fieldName() << ";";
                    }
                    cpp << "}";
                }
                cpp << EmptyLine;
            }
        }

        void writeParentPointerGetters(IndentingStreamWrapper& hpp, const Class& c) const {
            if (requiresParentPointer(c)) {
                if (c.deps.parents.size() > 1) {
                    hpp << "template<typename P>";
                    hpp << "bool IsParent() const";
                    hpp << "{";
                    {
                        Scope s(hpp);
                        hpp << "return m_parentType != NULL && *m_parentType == typeid(P);";
                    }
                    hpp << "}";
                    hpp << EmptyLine;
                }

                for (auto isConst : { false, true }) {
                    if (c.deps.parents.size() > 1) {
                        hpp << "template<typename P>";
                        if (isConst)
                            hpp << "const P* GetParent() const";
                        else
                            hpp << "P* GetParent()";
                        hpp << "{";
                        {
                            Scope s(hpp);
                            hpp << "static_assert(";
                            for (const auto& dep : c.deps.parents) {
                                if (&dep != &c.deps.parents[0])
                                    hpp.contLine() << " || ";
                                hpp.contLine() << "std::is_same<P, " << customReplacedType(dep->name) << ">::value";
                            }
                            hpp.contLine() << ", \"template argument for P is not a parent class of " << c.name << "\");";
                            if (c_generateDefaultCtorsForParentPointerTypes) {
                                hpp << "if (m_parent == NULL) {";
                                {
                                    Scope s(hpp);
                                    hpp << "return NULL;";
                                }
                                hpp << "}";
                            }
                            hpp << "if (!IsParent<P>()) {";
                            {
                                Scope s(hpp);
                                hpp << "throw CTiglError(\"bad parent\");";
                            }
                            hpp << "}";
                            hpp << "return static_cast<P*>(m_parent);";
                        }
                        hpp << "}";
                    }
                    else if (c.deps.parents.size() == 1) {
                        if (isConst)
                            hpp << "TIGL_EXPORT const " << customReplacedType(c.deps.parents[0]->name) << "* GetParent() const;";
                        else
                            hpp << "TIGL_EXPORT " << customReplacedType(c.deps.parents[0]->name) << "* GetParent();";
                    }
                    hpp << EmptyLine;
                }
            }
        }

        void writeUidManagerGetters(IndentingStreamWrapper& hpp, const Class& c) const {
            hpp << "TIGL_EXPORT virtual CTiglUIDObject* GetNextUIDParent();";
            hpp << "TIGL_EXPORT virtual const CTiglUIDObject* GetNextUIDParent() const;";
            hpp << EmptyLine;
            if (requiresUidManagerField(c)) {
                hpp << "TIGL_EXPORT " << c_uidMgrName << "& GetUIDManager();";
                hpp << "TIGL_EXPORT const " << c_uidMgrName << "& GetUIDManager() const;";
                hpp << EmptyLine;
            }
        }

        void writeParentPointerGetterImplementation(IndentingStreamWrapper& cpp, const Class& c) const {
            if (requiresParentPointer(c)) {
                if (c.deps.parents.size() == 1) {
                    for (auto isConst : { true, false }) {
                        if (isConst)
                            cpp << "const " << customReplacedType(c.deps.parents[0]->name) << "* " << c.name << "::GetParent() const";
                        else
                            cpp << customReplacedType(c.deps.parents[0]->name) << "* " << c.name << "::GetParent()";
                        cpp << "{";
                        {
                            Scope s(cpp);
                            cpp << "return m_parent;";
                        }
                        cpp << "}";
                        cpp << EmptyLine;
                    }
                }
            }
        }

        void writeUidManagerGetterImplementation(IndentingStreamWrapper& cpp, const Class& c) const {
            for (auto isConst : { true, false }) {
                if (isConst)
                    cpp << "const CTiglUIDObject* " << c.name << "::GetNextUIDParent() const";
                else
                    cpp << "CTiglUIDObject* " << c.name << "::GetNextUIDParent()";
                cpp << "{";
                {
                    Scope s(cpp);
                    if (c.deps.parents.size() == 0)
                        cpp << "return nullptr;";
                    else if (c.deps.parents.size() == 1) {
                        if (hasMandatoryUidField(*c.deps.parents[0]))
                            cpp << "return m_parent;";
                        else {
                            cpp << "if (m_parent) {";
                            {
                                Scope s(cpp);
                                if (hasUidField(*c.deps.parents[0])) {
                                    cpp << "if (m_parent->GetUID())";
                                    {
                                        Scope s(cpp);
                                        cpp << "return m_parent;";
                                    }
                                    cpp << "else";
                                    {
                                        Scope s(cpp);
                                        cpp << "return m_parent->GetNextUIDParent();";
                                    }
                                }
                                else {
                                    cpp << "return m_parent->GetNextUIDParent();";
                                }
                            }
                            cpp << "}";
                            cpp << "return nullptr;";
                        }
                    }
                    else {
                        cpp << "if (m_parent) {";
                        {
                            Scope s(cpp);
                            for (const auto& parentType : c.deps.parents) {
                                cpp << "if (IsParent<" << customReplacedType(parentType->name) << ">()) {";
                                {
                                    Scope s(cpp);
                                    if (hasMandatoryUidField(*parentType))
                                        cpp << "return GetParent<" << customReplacedType(parentType->name) << ">();";
                                    else if (hasUidField(*parentType)) {
                                        cpp << "if (GetParent<" << customReplacedType(parentType->name) << ">()->GetUID())";
                                        {
                                            Scope s(cpp);
                                            cpp << "return GetParent<" << customReplacedType(parentType->name) << ">();";
                                        }
                                        cpp << "else";
                                        {
                                            Scope s(cpp);
                                            cpp << "return GetParent<" << customReplacedType(parentType->name) << ">()->GetNextUIDParent();";
                                        }
                                    }
                                    else {
                                        cpp << "return GetParent<" << customReplacedType(parentType->name) << ">()->GetNextUIDParent();";
                                    }
                                }
                                cpp << "}";
                            }
                        }
                        cpp << "}";
                        cpp << "return nullptr;";
                    }
                }
                cpp << "}";
                cpp << EmptyLine;
            }

            if (requiresUidManagerField(c)) {
                cpp << "" << c_uidMgrName << "& " << c.name << "::GetUIDManager()";
                cpp << "{";
                {
                    Scope s(cpp);
                    cpp << "if (!m_uidMgr) {";
                    {
                        Scope s(cpp);
                        cpp << "throw CTiglError(\"UIDManager is null\");";
                    }
                    cpp << "}";
                    cpp << "return *m_uidMgr;";
                }
                cpp << "}";
                cpp << EmptyLine;

                cpp << "const " << c_uidMgrName << "& " << c.name << "::GetUIDManager() const";
                cpp << "{";
                {
                    Scope s(cpp);
                    cpp << "if (!m_uidMgr) {";
                    {
                        Scope s(cpp);
                        cpp << "throw CTiglError(\"UIDManager is null\");";
                    }
                    cpp << "}";
                    cpp << "return *m_uidMgr;";
                }
                cpp << "}";
                cpp << EmptyLine;
            }
        }

        void writeIODeclarations(IndentingStreamWrapper& hpp) const {
            hpp << "TIGL_EXPORT virtual void ReadCPACS(const TixiDocumentHandle& tixiHandle, const std::string& xpath);";
            hpp << "TIGL_EXPORT virtual void WriteCPACS(const TixiDocumentHandle& tixiHandle, const std::string& xpath) const;";
            hpp << EmptyLine;
        }

        void writeChoiceValidatorDeclaration(IndentingStreamWrapper& hpp, const Class& c) const {
            if (!c.choices.empty()) {
                hpp << "TIGL_EXPORT bool ValidateChoices() const;";
                hpp << EmptyLine;
            }
        }

        auto isAttribute(const XMLConstruct& construct) const -> bool {
            switch (construct) {
                case XMLConstruct::Attribute:
                    return true;
                case XMLConstruct::Element:
                case XMLConstruct::SimpleContent:
                case XMLConstruct::FundamentalTypeBase:
                    return false;
                default:
                    throw std::logic_error("Unknown XML construct");
            }
        }

        auto xmlConstructToString(const XMLConstruct& construct) const -> std::string {
            switch (construct) {
                case XMLConstruct::Attribute:           return "attribute";
                case XMLConstruct::Element:             return "element";
                case XMLConstruct::SimpleContent:       return "simpleContent";
                case XMLConstruct::FundamentalTypeBase: return "fundamental type base class";
                default: throw std::logic_error("Unknown XML construct");
            }
        }

        auto ctorArgumentList(const Class& c, const Class& parentClass) const -> std::string {
            std::vector<std::string> arguments;
            if (requiresParentPointer(c))
                arguments.push_back(parentPointerThis(parentClass));
            if (requiresUidManager(c))
                arguments.push_back("m_uidMgr");
            return boost::join(arguments, ", ");
        }

        void writeReadAttributeOrElementImplementation(IndentingStreamWrapper& cpp, const Class& c, const Field& f) const {
            const bool isAtt = isAttribute(f.xmlType);

            // fundamental types
            if (m_tables.m_fundamentalTypes.contains(f.typeName)) {
                switch (f.cardinality()) {
                    case Cardinality::Optional:
                    case Cardinality::Mandatory:
                        if (isAtt)
                            cpp << f.fieldName() << " = " << tixiHelperNamespace << "::TixiGetAttribute<" << f.typeName << ">(tixiHandle, xpath, \"" + f.cpacsName + "\");";
                        else {
                            const auto empty = f.xmlType == XMLConstruct::SimpleContent || f.xmlType == XMLConstruct::FundamentalTypeBase;
                            cpp << f.fieldName() << " = " << tixiHelperNamespace << "::TixiGetElement<" << f.typeName << ">(tixiHandle, xpath" << (empty ? "" : " + \"/" + f.cpacsName + "\"") << ");";
                        }

                        // check that mandatory string fields are not empty
                        if (f.cardinality() == Cardinality::Mandatory && f.typeName == "std::string") {
                            cpp << "if (" << f.fieldName() << ".empty()) {";
                            {
                                Scope s(cpp);
                                cpp << "LOG(WARNING) << \"Required "  << (isAtt ? "attribute " : "element ") << f.cpacsName << " is empty at xpath \" << xpath;";
                            }
                            cpp << "}";
                        }

                        // check that optional string fields are not empty
                        if (f.cardinality() == Cardinality::Optional && f.typeName == "std::string") {
                            cpp << "if (" << f.fieldName() << "->empty()) {";
                            {
                                Scope s(cpp);
                                cpp << "LOG(WARNING) << \"Optional "  << (isAtt ? "attribute " : "element ") << f.cpacsName << " is present but empty at xpath \" << xpath;";
                            }
                            cpp << "}";
                        }

                        // register uid references
                        if (f.xmlTypeName == c_uidRefType) {
                            if (f.cardinality() == Cardinality::Optional) {
                                cpp << "if (m_uidMgr && !" << f.fieldName() << "->empty()) m_uidMgr->RegisterReference(*" << f.fieldName() << ", *this);";
                            }
                            else {
                                cpp << "if (m_uidMgr && !" << f.fieldName() << ".empty()) m_uidMgr->RegisterReference(" << f.fieldName() << ", *this);";
                            }
                        }

                        break;
                    case Cardinality::Vector:
                        if (f.xmlType == XMLConstruct::Attribute || f.xmlType == XMLConstruct::SimpleContent || f.xmlType == XMLConstruct::FundamentalTypeBase)
                            throw std::runtime_error("Attributes, simpleContents and bases cannot be vectors");
                        assert(!isAtt);
                        cpp << tixiHelperNamespace << "::TixiReadElements(tixiHandle, xpath + \"/" << f.cpacsName << "\", " << f.fieldName() << ", " << f.minOccurs << ", " << formatMaxOccurs(f.maxOccurs) << ");";
                        // register uid references
                        if (f.xmlTypeName == c_uidRefType) {
                            cpp << "if (m_uidMgr) {";
                            {
                                Scope s(cpp);
                                cpp << "for (std::vector<" << f.typeName << ">::iterator it = " << f.fieldName() << ".begin(); it != " << f.fieldName() << ".end(); ++it) {";
                                {
                                    Scope s(cpp);
                                    cpp << "if (!it->empty()) m_uidMgr->RegisterReference(*it, *this);";
                                }
                                cpp << "}";
                            }
                            cpp << "}";
                        }
                        break;
                }

                return;
            }

            // enums
            const auto itE = m_types.enums.find(f.typeName);
            if (itE != std::end(m_types.enums)) {
                const auto& readFunc = stringToEnumFunc(itE->second, m_tables);
                switch (f.cardinality()) {
                    case Cardinality::Optional:
                    case Cardinality::Mandatory:
                        if (isAtt)
                            cpp << f.fieldName() << " = " << readFunc << "(" << tixiHelperNamespace << "::TixiGetAttribute<std::string>(tixiHandle, xpath, \"" + f.cpacsName + "\"));";
                        else
                            cpp << f.fieldName() << " = " << readFunc << "(" << tixiHelperNamespace << "::TixiGetElement<std::string>(tixiHandle, xpath + \"/" + f.cpacsName + "\"));";
                        break;
                    case Cardinality::Vector:
                        throw NotImplementedException("Reading enum vectors is not implemented");
                }
                return;
            }

            // classes
            const auto itC = m_types.classes.find(f.typeName);
            if (itC != std::end(m_types.classes)) {
                if (f.xmlType == XMLConstruct::Attribute || f.xmlType == XMLConstruct::FundamentalTypeBase)
                    throw std::logic_error("fields of class type cannot be attributes or fundamental type bases");

                switch (f.cardinality()) {
                    case Cardinality::Optional:
                        cpp << f.fieldName() << " = boost::in_place(" << ctorArgumentList(itC->second, c) << ");";
                        if (c_generateTryCatchAroundOptionalClassReads) {
                            cpp << "try {";
                            {
                                Scope s(cpp);
                                cpp << f.fieldName() << "->ReadCPACS(tixiHandle, xpath + \"/" << f.cpacsName << "\");";
                            }
                            cpp << "} catch(const std::exception& e) {";
                            {
                                Scope s(cpp);
                                cpp << "LOG(ERROR) << \"Failed to read " << f.cpacsName << " at xpath \" << xpath << \": \" << e.what();";
                                cpp << f.fieldName() << " = boost::none;";
                            }
                            cpp << "}";
                        } else
                            cpp << f.fieldName() << "->ReadCPACS(tixiHandle, xpath + \"/" << f.cpacsName << "\");";
                        break;
                    case Cardinality::Mandatory:
                        cpp << f.fieldName() << ".ReadCPACS(tixiHandle, xpath + \"/" + f.cpacsName + "\");";
                        break;
                    case Cardinality::Vector:
                        const auto moreArgs = ctorArgumentList(itC->second, c);
                        cpp << tixiHelperNamespace << "::TixiReadElements(tixiHandle, xpath + \"/" << f.cpacsName << "\", " << f.fieldName() << ", " << f.minOccurs << ", " << formatMaxOccurs(f.maxOccurs) << (moreArgs.empty() ? "" : ", " + moreArgs) << ");";
                        break;
                }
                return;
            }

            throw std::logic_error("No read function provided for type " + f.typeName);
        }

        void writeWriteAttributeOrElementImplementation(IndentingStreamWrapper& cpp, const Field& f, bool inSequence) const {
            const auto isAtt = isAttribute(f.xmlType);
            const auto empty = f.xmlType == XMLConstruct::SimpleContent || f.xmlType == XMLConstruct::FundamentalTypeBase;

            auto createElement = [&] {
                if (!empty && !isAtt)
                    if (inSequence)
                        cpp << tixiHelperNamespace << "::TixiCreateSequenceElementIfNotExists(tixiHandle, xpath + \"/" + f.cpacsName + "\", childElemOrder);";
                    else
                        cpp << tixiHelperNamespace << "::TixiCreateElementIfNotExists(tixiHandle, xpath + \"/" + f.cpacsName + "\");";
            };

            auto writeOptionalAttributeOrElement = [&](std::function<void()> writeReadFunc) { // make parameter auto when C++14 available
                cpp << "if (" << f.fieldName() << ") {";
                {
                    Scope s(cpp);
                    createElement();
                    writeReadFunc();
                }
                cpp << "}";
                cpp << "else {";
                {
                    Scope s(cpp);
                    if (isAtt)
                        cpp << "if (" << tixiHelperNamespace << "::TixiCheckAttribute(tixiHandle, xpath, \"" + f.cpacsName + "\")) {";
                    else
                        cpp << "if (" << tixiHelperNamespace << "::TixiCheckElement(tixiHandle, xpath" << (empty ? "" : " + \"/" + f.cpacsName + "\"") << ")) {";
                    {
                        Scope s(cpp);
                        if (isAtt)
                            cpp << tixiHelperNamespace << "::TixiRemoveAttribute(tixiHandle, xpath, \"" + f.cpacsName + "\");";
                        else
                            cpp << tixiHelperNamespace << "::TixiRemoveElement(tixiHandle, xpath" << (empty ? "" : " + \"/" + f.cpacsName + "\"") << ");";
                    }
                    cpp << "}";
                }
                cpp << "}";
            };

            // fundamental types
            if (m_tables.m_fundamentalTypes.contains(f.typeName)) {
                switch (f.cardinality()) {
                    case Cardinality::Optional:
                        writeOptionalAttributeOrElement([&] {
                            if (isAtt)
                                cpp << tixiHelperNamespace << "::TixiSaveAttribute(tixiHandle, xpath, \"" + f.cpacsName + "\", *" << f.fieldName() << ");";
                            else
                                cpp << tixiHelperNamespace << "::TixiSaveElement(tixiHandle, xpath" << (empty ? "" : " + \"/" + f.cpacsName + "\"") << ", *" << f.fieldName() << ");";
                        });
                        break;
                    case Cardinality::Mandatory:
                        createElement();
                        if (isAtt)
                            cpp << tixiHelperNamespace << "::TixiSaveAttribute(tixiHandle, xpath, \"" + f.cpacsName + "\", " << f.fieldName() << ");";
                        else
                            cpp << tixiHelperNamespace << "::TixiSaveElement(tixiHandle, xpath" << (empty ? "" : " + \"/" + f.cpacsName + "\"") << ", " << f.fieldName() << ");";
                        break;
                    case Cardinality::Vector:
                        if (f.xmlType == XMLConstruct::Attribute || f.xmlType == XMLConstruct::SimpleContent || f.xmlType == XMLConstruct::FundamentalTypeBase)
                            throw std::runtime_error("Attributes, simpleContents and bases cannot be vectors");
                        assert(!isAtt);
                        cpp << tixiHelperNamespace << "::TixiSaveElements(tixiHandle, xpath + \"/" << f.cpacsName << "\", " << f.fieldName() << ");";
                        break;
                }

                return;
            }

            // enums
            const auto itE = m_types.enums.find(f.typeName);
            if (itE != std::end(m_types.enums)) {
                switch (f.cardinality()) {
                    case Cardinality::Optional:
                        writeOptionalAttributeOrElement([&] {
                            cpp << tixiHelperNamespace << "::TixiSave" << (isAtt ? "Attribute" : "Element") << "(tixiHandle, xpath" << (isAtt ? ", \"" : " + \"/") << f.cpacsName + "\", " << enumToStringFunc(itE->second, m_tables) << "(*" << f.fieldName() << "));";
                        });
                        break;
                    case Cardinality::Mandatory:
                        createElement();
                        cpp << tixiHelperNamespace << "::TixiSave" << (isAtt ? "Attribute" : "Element") << "(tixiHandle, xpath" << (isAtt ? ", \"" : " + \"/") << f.cpacsName + "\", " << enumToStringFunc(itE->second, m_tables) << "(" << f.fieldName() << "));";
                        break;
                    case Cardinality::Vector:
                        throw NotImplementedException("Writing enum vectors is not implemented");
                }
                return;
            }

            // classes
            if (f.xmlType != XMLConstruct::Attribute && f.xmlType != XMLConstruct::FundamentalTypeBase) {
                const auto itC = m_types.classes.find(f.typeName);
                if (itC != std::end(m_types.classes)) {
                    switch (f.cardinality()) {
                        case Cardinality::Optional:
                            writeOptionalAttributeOrElement([&] {
                                cpp << f.fieldName() << "->WriteCPACS(tixiHandle, xpath + \"/" << f.cpacsName << "\");";
                            });
                            break;
                        case Cardinality::Mandatory:
                            createElement();
                            cpp << f.fieldName() << ".WriteCPACS(tixiHandle, xpath + \"/" + f.cpacsName + "\");";
                            break;
                        case Cardinality::Vector:
                            cpp << tixiHelperNamespace << "::TixiSaveElements(tixiHandle, xpath + \"/" << f.cpacsName << "\", " << f.fieldName() << ");";
                            break;
                    }
                    return;
                }
            }

            throw std::logic_error("No write function provided for type " + f.typeName);
        }

        void writeReadBaseImplementation(IndentingStreamWrapper& cpp, const std::string& type) const {
            // fundamental types
            if (m_tables.m_fundamentalTypes.contains(type))
                throw std::logic_error("fundamental types cannot be base classes"); // this should be prevented by TypeSystemBuilder

            // classes
            const auto itC = m_types.classes.find(type);
            if (itC != std::end(m_types.classes)) {
                cpp << type << "::ReadCPACS(tixiHandle, xpath);";
                return;
            }

            throw std::logic_error("No read function provided for type " + type);
        }

        void writeWriteBaseImplementation(IndentingStreamWrapper& cpp, const std::string& type) const {
            // fundamental types
            if (m_tables.m_fundamentalTypes.contains(type)) {
                cpp << tixiHelperNamespace << "::TixiSaveElement(tixiHandle, xpath, *this);";
                return;
            }

            // classes
            const auto itC = m_types.classes.find(type);
            if (itC != std::end(m_types.classes)) {
                cpp << type << "::WriteCPACS(tixiHandle, xpath);";
                return;
            }

            throw std::logic_error("No write function provided for type " + type);
        }

        void writeReadImplementation(IndentingStreamWrapper& cpp, const Class& c, const std::vector<Field>& fields) const {
            cpp << "void " << c.name << "::ReadCPACS(const TixiDocumentHandle& tixiHandle, const std::string& xpath)";
            cpp << "{";
            {
                Scope s(cpp);

                // base class
                if (!c.base.empty()) {
                    cpp << "// read base";
                    writeReadBaseImplementation(cpp, c.base);
                    cpp << EmptyLine;
                }

                // fields
                for (const auto& f : fields) {
                    const auto construct = xmlConstructToString(f.xmlType);
                    const auto isAtt = isAttribute(f.xmlType);
                    cpp << "// read " << construct << " " << f.cpacsName << "";
                    if (isAtt)
                        cpp << "if (" << tixiHelperNamespace << "::TixiCheckAttribute(tixiHandle, xpath, \"" << f.cpacsName << "\")) {";
                    else {
                        const auto empty = f.xmlType == XMLConstruct::SimpleContent || f.xmlType == XMLConstruct::FundamentalTypeBase;
                        cpp << "if (" << tixiHelperNamespace << "::TixiCheckElement(tixiHandle, xpath" << (empty ? "" : " + \"/" + f.cpacsName + "\"") << ")) {";
                    }
                    {
                        Scope s(cpp);
                        writeReadAttributeOrElementImplementation(cpp, c, f);
                    }
                    cpp << "}";
                    if (f.cardinality() == Cardinality::Mandatory) {
                        // attribute or element must exist
                        cpp << "else {";
                        {
                            Scope s(cpp);
                            cpp << "LOG(ERROR) << \"Required " << construct << " " << f.cpacsName << " is missing at xpath \" << xpath;";
                        }
                        cpp << "}";
                    }
                    cpp << EmptyLine;
                }

                // register
                if (hasUidField(c)) {
                    if (hasMandatoryUidField(c))
                        cpp << "if (m_uidMgr && !m_uID.empty()) m_uidMgr->RegisterObject(m_uID, *this);";
                    else
                        cpp << "if (m_uidMgr && m_uID) m_uidMgr->RegisterObject(*m_uID, *this);";
                }

                // validate choices
                if (!c.choices.empty()) {
                    cpp << "if (!ValidateChoices()) {";
                    {
                        Scope s(cpp);
                        cpp << "LOG(ERROR) << \"Invalid choice configuration at xpath \" << xpath;";
                    }
                    cpp << "}";
                }
            }
            cpp << "}";
            cpp << EmptyLine;
        }

        void writeWriteImplementation(IndentingStreamWrapper& cpp, const Class& c, const std::vector<Field>& fields) const {
            cpp << "void " << c.name << "::WriteCPACS(const TixiDocumentHandle& tixiHandle, const std::string& xpath) const";
            cpp << "{";
            {
                Scope s(cpp);

                // NOTE: only handling sequences when no choice is contained, or when all elements in choices are
                //       unique (same elements don't appear in multiple choices)
                const bool handleSequence = c.containsSequence && (c.choices.empty() || uniqueChildElements(c));
                if (handleSequence) {
                    writeChildElemOrder(cpp, c);
                }

                // base class
                if (!c.base.empty()) {
                    cpp << "// write base";
                    writeWriteBaseImplementation(cpp, c.base);
                    cpp << EmptyLine;
                }

                // fields
                for (const auto& f : fields) {
                    const auto construct = xmlConstructToString(f.xmlType);
                    cpp << "// write " << construct << " " << f.cpacsName;
                    // NOTE: only handling sequences when no choice is contained, since determination of order
                    //       would get more complex otherwise
                    writeWriteAttributeOrElementImplementation(cpp, f, handleSequence);
                    cpp << EmptyLine;
                }
            }
            cpp << "}";
            cpp << EmptyLine;
        }

        static auto writeIsFieldThere(IndentingStreamWrapper& cpp, const Field& f) {
            if (f.cardinality() == Cardinality::Optional)
                cpp << f.fieldName() + ".is_initialized()";
            else if (f.cardinality() == Cardinality::Vector)
                cpp << "!" + f.fieldName() + ".empty()";
            else
                throw std::logic_error("elements inside choice can only be optional or vector");
        }

        void writeChoiceValidatorImplementation(IndentingStreamWrapper& cpp, const Class& c) const {
            if (!c.choices.empty()) {
                cpp << "bool " << c.name << "::ValidateChoices() const";
                cpp << "{";
                {
                    Scope s(cpp);



                    struct RecursiveColletor : public boost::static_visitor<> {
                        void operator()(const ChoiceElement& ce) {
                            indices.push_back(ce.index);
                        }

                        void operator()(const Choice& c) {
                            for (const auto& cc : c.options)
                                (*this)(cc);
                        }

                        void operator()(const ChoiceElements& ces) {
                            for (const auto& ce : ces)
                                ce.apply_visitor(*this);
                        }

                        std::vector<std::size_t> indices;
                    };

                    struct ChoiceWriter : public boost::static_visitor<> {
                        ChoiceWriter(IndentingStreamWrapper& cpp, const Class& c)
                            : cpp(cpp), c(c) {}

                        void operator()(const ChoiceElement& ce) {
                            const auto& f = c.fields[ce.index];
                            if (!ce.optionalBefore)
                                writeIsFieldThere(cpp, f);
                            else
                                cpp << "true // " << f.fieldName() << " is optional in choice";
                        }

                        void operator()(const Choice& ch) {
                            cpp << "(";
                            {
                                Scope s(cpp);
                                boost::optional<Scope> additionalScope;

                                if (ch.minOccurs == 0) {
                                  cpp << "// all uninitialized is valid since choice is optional!";
                                  cpp << "!(";
                                    {
                                        Scope s(cpp);

                                        RecursiveColletor parentCollector;
                                        parentCollector(ch);
                                        auto& allIndices = parentCollector.indices;

                                        auto unique = [](std::vector<std::size_t>& v) {
                                            std::sort(std::begin(v), std::end(v));
                                            const auto it = std::unique(std::begin(v), std::end(v));
                                            v.erase(it, std::end(v));
                                        };
                                        unique(allIndices);

                                        for (const auto& i : allIndices) {
                                            writeIsFieldThere(cpp, c.fields[i]);
                                            if (&i != &allIndices.back())
                                                cpp << "||";
                                        }
                                    }
                                    cpp << ")";
                                    cpp << "||";
                                    cpp << "(";
                                    additionalScope.emplace(cpp);
                                }

                                for (const auto &ces : ch.options) {
                                    (*this)(ces, ch);
                                    if (&ces != &ch.options.back())
                                        cpp << "+";
                                }
                                cpp << "== 1";

                                if (additionalScope) {
                                  additionalScope.reset();
                                  cpp << ")";
                                }
                            }
                            cpp << ")";
                        }

                        void operator()(const ChoiceElements& ces, boost::optional<const Choice&> parentChoice = {}) {
                            cpp << "(";
                            {
                                Scope s(cpp);

                                if (parentChoice)
                                    cpp << "// mandatory elements of this choice must be there";
                                for (const auto& ce : ces) {
                                    ce.apply_visitor(*this);
                                    if (&ce != &ces.back())
                                        cpp << "&&";
                                }

                                if (parentChoice) {
                                    cpp << "&&";
                                    cpp << "// elements of other choices must not be there";
                                    cpp << "!(";
                                    {
                                        Scope s(cpp);

                                        RecursiveColletor parentCollector;
                                        parentCollector(*parentChoice);
                                        auto& allIndices = parentCollector.indices;

                                        RecursiveColletor childCollector;
                                        childCollector(ces);
                                        auto& childIndices = childCollector.indices;

                                        auto unique = [](std::vector<std::size_t>& v) {
                                            std::sort(std::begin(v), std::end(v));
                                            const auto it = std::unique(std::begin(v), std::end(v));
                                            v.erase(it, std::end(v));
                                        };
                                        unique(allIndices);
                                        unique(childIndices);

                                        const auto it = std::remove_if(std::begin(allIndices), std::end(allIndices), [&](std::size_t ai) {
                                            for (const auto ci : childIndices) {
                                                if (ci == ai)
                                                    return true;
                                                // additionally exclude elements with the same name in cpacs (choices with the same element in both alternatives)
                                                if (c.fields[ci].cpacsName == c.fields[ai].cpacsName)
                                                    return true;
                                            }
                                            return false;
                                        });
                                        allIndices.erase(it, std::end(allIndices));

                                        for (const auto& i : allIndices) {
                                            writeIsFieldThere(cpp, c.fields[i]);
                                            if (&i != &allIndices.back())
                                                cpp << "||";
                                        }
                                    }
                                    cpp << ")";
                                }
                            }
                            cpp << ")";
                        }

                    private:
                        IndentingStreamWrapper& cpp;
                        const Class& c;
                    };

                    cpp << "return";
                    ChoiceWriter{cpp, c}(c.choices);
                    cpp << ";";
                }
                cpp << "}";
                cpp << EmptyLine;
            }
        }

        void writeTreeManipulatorDeclarations(IndentingStreamWrapper& hpp, const std::vector<Field>& fields) const {
            for (const auto& f : fields) {
                if (m_types.classes.find(f.typeName) != std::end(m_types.classes)) {
                    if (f.xmlType == XMLConstruct::Attribute || f.xmlType == XMLConstruct::FundamentalTypeBase)
                        throw std::logic_error("fields of class type cannot be attributes or fundamental type bases");

                    switch (f.cardinality()) {
                        case Cardinality::Optional:
                            hpp << "TIGL_EXPORT virtual " << customReplacedType(f) << "& Get" << capitalizeFirstLetter(f.name()) << "(CreateIfNotExistsTag);";
                            hpp << "TIGL_EXPORT virtual void Remove" << capitalizeFirstLetter(f.name()) << "();";
                            hpp << EmptyLine;
                            break;
                        case Cardinality::Vector:
                            hpp << "TIGL_EXPORT virtual " << customReplacedType(f) << "& Add" << capitalizeFirstLetter(f.nameWithoutVectorS()) << "();";
                            hpp << "TIGL_EXPORT virtual void Remove" << capitalizeFirstLetter(f.nameWithoutVectorS()) << "(" <<  customReplacedType(f) << "& ref);";
                            hpp << EmptyLine;
                            break;
                    }
                }
            }
        }

        void writeTreeManipulatorImplementations(IndentingStreamWrapper& cpp, const Class& c) const {
            for (const auto& f : c.fields) {
                const auto itC = m_types.classes.find(f.typeName);
                if (itC != std::end(m_types.classes)) {
                    if (f.xmlType == XMLConstruct::Attribute || f.xmlType == XMLConstruct::FundamentalTypeBase)
                        throw std::logic_error("fields of class type cannot be attributes or fundamental type bases");

                    switch (f.cardinality()) {
                        case Cardinality::Optional:
                            cpp << "" << customReplacedType(f) << "& " << c.name << "::Get" << capitalizeFirstLetter(f.name()) << "(CreateIfNotExistsTag)";
                            cpp << "{";
                            {
                                Scope s(cpp);
                                cpp << "if (!" << f.fieldName() << ")";
                                {
                                    Scope s(cpp);
                                    cpp << f.fieldName() << " = boost::in_place(" << ctorArgumentList(itC->second, c) << ");";
                                }
                                cpp << "return *" << f.fieldName() << ";";
                            }
                            cpp << "}";
                            cpp << EmptyLine;
                            cpp << "void " << c.name << "::Remove" << capitalizeFirstLetter(f.name()) << "()";
                            cpp << "{";
                            {
                                Scope s(cpp);
                                cpp << f.fieldName() << " = boost::none;";
                            }
                            cpp << "}";
                            cpp << EmptyLine;
                            break;
                        case Cardinality::Vector:
                            cpp << "" << customReplacedType(f) << "& " << c.name << "::Add" << capitalizeFirstLetter(f.nameWithoutVectorS()) << "()";
                            cpp << "{";
                            {
                                Scope s(cpp);
                                cpp << f.fieldName() << ".push_back(make_unique<" << customReplacedType(f) << ">(" << ctorArgumentList(itC->second, c) << "));";
                                cpp << "return *" << f.fieldName() << ".back();";
                            }
                            cpp << "}";
                            cpp << EmptyLine;
                            cpp << "void " << c.name << "::Remove" << capitalizeFirstLetter(f.nameWithoutVectorS()) << "(" <<  customReplacedType(f) << "& ref)";
                            cpp << "{";
                            {
                                Scope s(cpp);
                                cpp << "for (std::size_t i = 0; i < " << f.fieldName() << ".size(); i++) {";
                                {
                                    Scope s(cpp);
                                    cpp << "if (" << f.fieldName() << "[i].get() == &ref) {";
                                    {
                                        Scope s(cpp);
                                        cpp << f.fieldName() << ".erase(" << f.fieldName() << ".begin() + i);";
                                        cpp << "return;";
                                    }
                                    cpp << "}";
                                }
                                cpp << "}";
                                cpp << "throw CTiglError(\"Element not found\");";
                            }
                            cpp << "}";
                            cpp << EmptyLine;
                            break;
                    }
                }
            }
        }

        void writeUidRefObjectFunctionDeclaractions(IndentingStreamWrapper& hpp) const {
            hpp << "TIGL_EXPORT const CTiglUIDObject* GetNextUIDObject() const final;";
            hpp << "TIGL_EXPORT void NotifyUIDChange(const std::string& oldUid, const std::string& newUid) final;";
            hpp << EmptyLine;
        }

        void writeUidRefObjectFunctionImplementations(IndentingStreamWrapper& cpp, const Class& c) const {
            cpp << "const CTiglUIDObject* " << c.name << "::GetNextUIDObject() const";
            cpp << "{";
            {
                Scope s(cpp);
                if (hasUidField(c))
                    cpp << "return this;";
                else
                    cpp << "return GetNextUIDParent();";
            }
            cpp << "}";
            cpp << EmptyLine;
            cpp << "void " << c.name << "::NotifyUIDChange(const std::string& oldUid, const std::string& newUid)";
            cpp << "{";
            {
                Scope s(cpp);
                for (const auto& f : uidReferenceFields(c)) {
                    if (f.cardinality() == Cardinality::Vector) {
                        cpp << "for (auto& entry : " << f.fieldName() << ") {";
                        {
                            Scope s(cpp);
                            cpp << "if (entry == oldUid) {";
                            {
                                Scope s(cpp);
                                cpp << "entry = newUid;";
                            }
                            cpp << "}";
                        }
                        cpp << "}";
                    }
                    else if (f.cardinality() == Cardinality::Optional) {
                        cpp << "if (" << f.fieldName() << " && *" << f.fieldName() << " == oldUid) {";
                        {
                            Scope s(cpp);
                            cpp << f.fieldName() << " = newUid;";
                        }
                        cpp << "}";
                    }
                    else {
                        cpp << "if (" << f.fieldName() << " == oldUid) {";
                        {
                            Scope s(cpp);
                            cpp << f.fieldName() << " = newUid;";
                        }
                        cpp << "}";
                    }
                }
            }
            cpp << "}";
            cpp << EmptyLine;
        }

        void writeLicenseHeader(IndentingStreamWrapper& f) const {
            f << "// Copyright (c) 2020 RISC Software GmbH";
            f << "//";
            f << "// This file was generated by CPACSGen from CPACS XML Schema (c) German Aerospace Center (DLR/SC).";
            f << "// Do not edit, all changes are lost when files are re-generated.";
            f << "//";
            f << "// Licensed under the Apache License, Version 2.0 (the \"License\")";
            f << "// you may not use this file except in compliance with the License.";
            f << "// You may obtain a copy of the License at";
            f << "//";
            f << "//     http://www.apache.org/licenses/LICENSE-2.0";
            f << "//";
            f << "// Unless required by applicable law or agreed to in writing, software";
            f << "// distributed under the License is distributed on an \"AS IS\" BASIS,";
            f << "// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.";
            f << "// See the License for the specific language governing permissions and";
            f << "// limitations under the License.";
            f << EmptyLine;
        }

        auto resolveIncludes(const Class& c) const -> Includes {
            Includes deps;

            deps.hppIncludes.push_back("<tixi.h>");
            deps.hppIncludes.push_back("<string>");
            deps.hppIncludes.push_back("\"tigl_internal.h\"");

            // optional, vector and make_unique
            bool vectorHeader = false;
            bool makeUnique = false;
            bool optionalHeader = false;
            bool createIfNotExistsHeader = false;
            bool timeHeader = false;
            for (const auto& f : c.fields) {
                switch (f.cardinality()) {
                    case Cardinality::Optional:
                        optionalHeader = true;
                        if (m_types.classes.find(f.typeName) != std::end(m_types.classes))
                            createIfNotExistsHeader = true;
                        break;
                    case Cardinality::Vector:
                        vectorHeader = true;
                        if (m_types.classes.find(f.typeName) != std::end(m_types.classes))
                            makeUnique = true;
                        break;
                    case Cardinality::Mandatory:
                        break;
                }
                if (f.typeName == "std::time_t")
                    timeHeader = true;
            }
            if (vectorHeader) {
                deps.hppIncludes.push_back("<vector>");
                if (makeUnique) {
                    deps.hppIncludes.push_back("\"UniquePtr.h\"");
                }
            }
            if (optionalHeader) {
                deps.hppIncludes.push_back("<boost/optional.hpp>");
                deps.hppIncludes.push_back("<boost/utility/in_place_factory.hpp>");
                if (createIfNotExistsHeader)
                    deps.hppIncludes.push_back("\"CreateIfNotExists.h\"");
            }
            if (timeHeader)
                deps.hppIncludes.push_back("<ctime>");
            if (c.deps.parents.size() > 1) {
                deps.hppIncludes.push_back("\"CTiglError.h\"");
                deps.hppIncludes.push_back("<typeinfo>");
            }
            if (requiresUidManager(c)) {
                deps.hppCustomForwards.push_back(c_uidMgrName);
                deps.cppIncludes.push_back("\"" + c_uidMgrName + ".h\"");
            }
            if (hasUidRefField(c)) {
                deps.hppIncludes.push_back("\"ITiglUIDRefObject.h\"");
            }
            if (hasUidField(c)) {
                deps.hppIncludes.push_back("\"CTiglUIDObject.h\"");
            }
            else {
                deps.hppCustomForwards.push_back("CTiglUIDObject");
                deps.cppIncludes.push_back("\"CTiglUIDObject.h\"");
            }

            // base class
            if (!c.base.empty() && m_types.classes.find(c.base) != std::end(m_types.classes)) {
                deps.hppIncludes.push_back("\"" + c.base + ".h\"");
            }

            // fields
            for (const auto& f : c.fields) {
                if (m_types.enums.find(f.typeName) != std::end(m_types.enums) ||
                    m_types.classes.find(f.typeName) != std::end(m_types.classes)) {
                    // this is a class or enum type, include it

                    const auto p = m_tables.m_customTypes.find(f.typeName);
                    if (!p) {
                        switch (f.cardinality()) {
                            case Cardinality::Optional:
                            case Cardinality::Mandatory:
                                deps.hppIncludes.push_back("\"" + f.typeName + ".h\"");
                                break;
                            case Cardinality::Vector:
                                deps.hppForwards.push_back(f.typeName);
                                deps.cppIncludes.push_back("\"" + f.typeName + ".h\"");
                                break;
                        }
                    } else {
                        // custom types are Tigl types and resolved using include paths and require a different namespace
                        switch (f.cardinality()) {
                            case Cardinality::Optional:
                            case Cardinality::Mandatory:
                                deps.hppIncludes.push_back("<" + *p + ".h>");
                                break;
                            case Cardinality::Vector:
                                deps.hppCustomForwards.push_back(*p);
                                deps.cppIncludes.push_back("<" + *p + ".h>");
                                break;
                        }
                    }
                }
            }

            // parent pointers
            if (requiresParentPointer(c)) {
                deps.cppIncludes.push_back("<cassert>");
                for (const auto& dep : c.deps.parents) {
                    const auto p = m_tables.m_customTypes.find(dep->name);
                    if (p) {
                        deps.hppCustomForwards.push_back(*p);
                        deps.cppIncludes.push_back("\"" + *p + ".h\"");
                    } else {
                        deps.hppForwards.push_back(dep->name);
                        deps.cppIncludes.push_back("\"" + dep->name + ".h\"");
                    }
                }
            }

            // misc cpp includes
            deps.cppIncludes.push_back("\"TixiHelper.h\"");
            deps.cppIncludes.push_back("\"CTiglLogging.h\"");
            deps.cppIncludes.push_back("\"CTiglError.h\""); // remove this, when CTiglError inherits std::exception
            deps.cppIncludes.push_back("\"" + c.name + ".h\"");

            // sort
            auto order = [](std::vector<std::string>& includes) {
                // split global from local includes
                const auto& mid = std::partition(std::begin(includes), std::end(includes), [](const std::string& s) {
                    return s[0] == '<';
                });
                // sort these groups individually
                auto icmp = [](const std::string& a, const std::string& b) { return boost::ilexicographical_compare(a, b); };
                std::sort(std::begin(includes), mid, icmp);
                std::sort(mid, std::end(includes), icmp);
                const auto& newMid = includes.erase(std::unique(std::begin(includes), mid), mid);
                includes.erase(std::unique(newMid, std::end(includes)), std::end(includes));
            };
            order(deps.hppIncludes);
            order(deps.cppIncludes);

            return deps;
        }

        void writeCtors(IndentingStreamWrapper& hpp, const Class& c) const {
            const auto hasUid = requiresUidManager(c);
            if (requiresParentPointer(c)) {
                if (c_generateDefaultCtorsForParentPointerTypes)
                    hpp << "TIGL_EXPORT " << c.name << "(" << (hasUid ? c_uidMgrName + "* uidMgr" : "") << ");";
                for (const auto& dep : c.deps.parents)
                    hpp << "TIGL_EXPORT " << c.name << "(" << customReplacedType(dep->name) << "* parent" << (hasUid ? ", " + c_uidMgrName + "* uidMgr" : "") << ");";
                hpp << EmptyLine;
            } else {
                hpp << "TIGL_EXPORT " << c.name << "(" << (hasUid ? c_uidMgrName + "* uidMgr" : "") << ");";
            }
        }

        void writeParentPointerFields(IndentingStreamWrapper& hpp, const Class& c) const {
            if (requiresParentPointer(c)) {
                if (c.deps.parents.size() > 1) {
                    hpp << "void* m_parent;";
                    hpp << "const std::type_info* m_parentType;";
                } else if (c.deps.parents.size() == 1) {
                    hpp << customReplacedType(c.deps.parents[0]->name) << "* m_parent;";
                }
                hpp << EmptyLine;
            }
        }

        void writeUidManagerFields(IndentingStreamWrapper& hpp, const Class& c) const {
            if (requiresUidManagerField(c)) {
                hpp << c_uidMgrName << "* m_uidMgr;";
                hpp << EmptyLine;
            }
        }

        void writeDeletedCTorAndAssign(IndentingStreamWrapper& hpp, const Class& c) const {
            hpp << c.name << "(const " << c.name << "&) = delete;";
            hpp << "" << c.name << "& operator=(const " << c.name << "&) = delete;";
            hpp << EmptyLine;
            hpp << c.name << "(" << c.name << "&&) = delete;";
            hpp << c.name << "& operator=(" << c.name << "&&) = delete;";
        }

        auto parentPointerThis(const Class& c) const -> std::string {
            const auto cust = m_tables.m_customTypes.find(c.name);
            if (cust)
                return "reinterpret_cast<" + *cust + "*>(this)";
            else
                return "this";
        }

        void writeCtorImplementations(IndentingStreamWrapper& cpp, const Class& c) const {
            const auto hasUidArgument = requiresUidManager(c);
            const auto hasUidMgrField = requiresUidManagerField(c);

            auto writeInitializationList = [&] {
                Scope s(cpp);
                bool first = true;
                auto writeBaseOrMember = [&](const std::string& baseOrMemberName, const std::string& value) {
                    if (first) {
                        cpp << ": ";
                        first = false;
                    } else
                        cpp << ", ";
                    cpp.contLine() << baseOrMemberName << "(" << value << ")";
                };
                for (const auto& bc : c.deps.bases) {
                    if (requiresUidManager(*bc)) {
                        writeBaseOrMember(bc->name, "uidMgr");
                    }
                }
                if (hasUidMgrField)
                    writeBaseOrMember("m_uidMgr", "uidMgr");
                for (const auto& f : c.fields) {
                    if (f.cardinality() == Cardinality::Mandatory) {
                        if (m_tables.m_fundamentalTypes.contains(f.typeName)) {
                            auto args = f.defaultValue;
                            if (!args.empty() && f.typeName == "std::string")
                                args = "\"" + args + "\"";
                            else if (args.empty() && f.typeName != "std::string")
                                args = "0"; // if the field has no default value and is a fundamental data type, but not string, provide zero initializer
                            if (!args.empty())
                                writeBaseOrMember(f.fieldName(), args);
                        } else {
                            const auto fci = m_types.classes.find(f.typeName);
                            if (fci != std::end(m_types.classes)) {
                                const auto args = ctorArgumentList(fci->second, c);
                                if (!args.empty())
                                    writeBaseOrMember(f.fieldName(), args);
                            }
                        }
                    }
                }
            };

            // if this class holds parent pointers, we have to provide corresponding ctor overloads
            if (requiresParentPointer(c)) {
                if (c_generateDefaultCtorsForParentPointerTypes) {
                    cpp << c.name << "::" << c.name << "(" << (hasUidArgument ? c_uidMgrName + "* uidMgr" : "") << ")";
                    writeInitializationList();
                    cpp << "{";
                    {
                        Scope s(cpp);
                        cpp << "m_parent = NULL;";
                        if (c.deps.parents.size() > 1)
                            cpp << "m_parentType = NULL;";
                    }
                    cpp << "}";
                    cpp << EmptyLine;
                }
                for (const auto& dep : c.deps.parents) {
                    const auto rn = customReplacedType(dep->name);
                    cpp << c.name << "::" << c.name << "(" << rn << "* parent" << (hasUidArgument ? ", " + c_uidMgrName + "* uidMgr" : "") << ")";
                    writeInitializationList();
                    cpp << "{";
                    {
                        Scope s(cpp);
                        cpp << "//assert(parent != NULL);";
                        cpp << "m_parent = parent;";
                        if (c.deps.parents.size() > 1)
                            cpp << "m_parentType = &typeid(" << rn << ");";
                    }
                    cpp << "}";
                    cpp << EmptyLine;
                }
            } else {
                cpp << c.name << "::" << c.name << "(" << (hasUidArgument ? c_uidMgrName + "* uidMgr" : "") << ")";
                writeInitializationList();
                cpp << "{";
                cpp << "}";
                cpp << EmptyLine;
            }
        }

        void writeDtor(IndentingStreamWrapper& hpp, const Class& c) const {
            hpp << "TIGL_EXPORT virtual ~" << c.name << "();";
            hpp << EmptyLine;
        }

        void writeDtorImplementation(IndentingStreamWrapper& cpp, const Class& c) const {
            auto writeUidRefUnregistration = [&] {
                const auto fields = uidReferenceFields(c);
                if (!fields.empty()) {
                    cpp << "if (m_uidMgr) {";
                    {
                        Scope s(cpp);
                        for (const auto& f : fields) {
                            if (f.cardinality() == Cardinality::Optional)
                                cpp << "if (" << f.fieldName() << " && !" << f.fieldName() << "->empty()) m_uidMgr->TryUnregisterReference(*" << f.fieldName() << ", *this);";
                            else if (f.cardinality() == Cardinality::Vector) {
                                cpp << "for (std::vector<" << f.typeName << ">::iterator it = " << f.fieldName() << ".begin(); it != " << f.fieldName() << ".end(); ++it) {";
                                {
                                    Scope s(cpp);
                                    cpp << "if (!it->empty()) m_uidMgr->TryUnregisterReference(*it, *this);";
                                }
                                cpp << "}";
                            }
                            else
                                cpp << "if (!" << f.fieldName() << ".empty()) m_uidMgr->TryUnregisterReference(" << f.fieldName() << ", *this);";
                        }
                    }
                    cpp << "}";
                }
            };

            cpp << c.name << "::~" << c.name << "()";
            cpp << "{";
            {
                Scope s(cpp);
                if (hasUidField(c)) {
                    if (hasMandatoryUidField(c))
                        cpp << "if (m_uidMgr) m_uidMgr->TryUnregisterObject(m_uID);";
                    else
                        cpp << "if (m_uidMgr && m_uID) m_uidMgr->TryUnregisterObject(*m_uID);";
                }
                writeUidRefUnregistration();
            }
            cpp << "}";
            cpp << EmptyLine;
        }

        void writeHeader(IndentingStreamWrapper& hpp, const Class& c, const Includes& includes) const {
            // file header
            writeLicenseHeader(hpp);

            hpp << "#pragma once";
            hpp << EmptyLine;

            // includes
            for (const auto& inc : includes.hppIncludes)
                hpp << "#include " << inc << "";
            if (includes.hppIncludes.size() > 0)
                hpp << EmptyLine;

            // namespace
            hpp << "namespace tigl";
            hpp << "{";
            {
                // custom Tigl types declarations
                for (const auto& fwd : includes.hppCustomForwards)
                    hpp << "class " << fwd << ";";
                if (includes.hppCustomForwards.size() > 0)
                    hpp << EmptyLine;

                hpp << "namespace generated";
                hpp << "{";
                {
                    Scope s(hpp);

                    boost::optional<Scope> ops;
                    if (!m_namespace.empty()) {
                        hpp << "namespace " << m_namespace;
                        hpp << "{";
                        ops = boost::in_place(std::ref(hpp));
                    }

                    // forward declarations
                    for (const auto& fwd : includes.hppForwards)
                        hpp << "class " << fwd << ";";
                    if (includes.hppForwards.size() > 0)
                        hpp << EmptyLine;

                    // meta information from schema
                    hpp << "// This class is used in:";
                    for (const auto& p : c.deps.parents)
                        hpp << "// " << p->name << "";
                    if (c.deps.parents.size() > 0)
                        hpp << EmptyLine;

                    // documentation
                    writeDocumentation(hpp, c.documentation);

                    // class name and base class
                    std::stringstream baseclause;
                    if (!c.base.empty() || hasUidField(c) || hasUidRefField(c)) {
                        if (!c.base.empty())
                            baseclause << "public " << c.base;
                        if (hasUidField(c))
                            baseclause << (baseclause.str().empty() ? "" : ", ") << "public " << (hasMandatoryUidField(c) ? "CTiglReqUIDObject" : "CTiglOptUIDObject");
                        if (hasUidRefField(c))
                            baseclause << (baseclause.str().empty() ? "" : ", ") << "public ITiglUIDRefObject";
                    }
                    if (baseclause.str().empty())
                        hpp << "class " << c.name;
                    else
                        hpp << "class " << c.name << " : " << baseclause.str();
                    hpp << "{";
                    hpp << "public:";
                    {
                        Scope s(hpp);

                        // ctor
                        writeCtors(hpp, c);

                        // dtor
                        writeDtor(hpp, c);

                        // parent pointers
                        writeParentPointerGetters(hpp, c);

                        // uid manager
                        writeUidManagerGetters(hpp, c);

                        // io
                        writeIODeclarations(hpp);

                        // choice validator
                        writeChoiceValidatorDeclaration(hpp, c);

                        // accessors
                        writeAccessorDeclarations(hpp, c.fields);

                        // tree manipulators
                        writeTreeManipulatorDeclarations(hpp, c.fields);
                    }
                    hpp << "protected:";
                    {
                        Scope s(hpp);

                        // parent pointers
                        writeParentPointerFields(hpp, c);

                        // uid manager
                        writeUidManagerFields(hpp, c);

                        // fields
                        writeFields(hpp, c.fields);
                    }
                    hpp << "private:";
                    {
                        Scope s(hpp);

                        if (hasUidRefField(c)) {
                            // write declarations for uid ref object functions
                            writeUidRefObjectFunctionDeclaractions(hpp);
                        }

                        // copy ctor and assign, move ctor and assign
                        writeDeletedCTorAndAssign(hpp, c);
                    }
                    hpp << "};";

                    if (!m_namespace.empty()) {
                        ops = boost::none;
                        hpp << "}";
                    }
                }

                hpp << "} // namespace generated";
                hpp << EmptyLine;

                // export non-custom types into tigl namespace
                const auto generatedNs = "generated" + (m_namespace.empty() ? "" : "::" + m_namespace);

                std::vector<std::string> exportedTypes;
                const auto& customName = m_tables.m_customTypes.find(c.name);
                if (customName) {
                    hpp << "// " << c.name << " is customized, use type " << *customName << " directly";
                    if (includes.hppForwards.size() > 0)
                        hpp << EmptyLine;
                } else
                    exportedTypes.push_back(c.name);

                for (const auto& fwd : includes.hppForwards)
                    exportedTypes.push_back(fwd);

                if (!exportedTypes.empty()) {
                    hpp << "// Aliases in tigl namespace";

                    boost::optional<Scope> ops;
                    if (!m_namespace.empty()) {
                        hpp << "namespace " << m_namespace;
                        hpp << "{";
                        ops = boost::in_place(std::ref(hpp));
                    }

                    for (const auto& name : exportedTypes)
                        hpp << "using C" << name << " = " << generatedNs << "::" << name << ";";

                    if (!m_namespace.empty()) {
                        ops = boost::none;
                        hpp << "}";
                    }
                }
            }
            hpp << "} // namespace tigl";
            hpp << EmptyLine;
        }

        void writeChildElemOrder(IndentingStreamWrapper& cpp, const Class& c) const {
            const auto elemNames = elementNames(c);
            cpp << "const std::vector<std::string> childElemOrder = { " << [&] {
                std::stringstream ss;
                for (const auto& e : elemNames) {
                    if (!ss.str().empty())
                        ss << ", ";
                    ss << "\"" << e << "\"";
                }
                return ss.str();
            }() << " };";
            cpp << EmptyLine;
        }

        void writeSource(IndentingStreamWrapper& cpp, const Class& c, const Includes& includes) const {
            // file header
            writeLicenseHeader(cpp);

            // includes
            for (const auto& inc : includes.cppIncludes)
                cpp << "#include " << inc << "";
            if (includes.cppIncludes.size() > 0)
                cpp << EmptyLine;

            // namespace
            cpp << "namespace tigl";
            cpp << "{";
            {
                cpp << "namespace generated";
                cpp << "{";
                {
                    Scope s(cpp);

                    boost::optional<Scope> ops;
                    if (!m_namespace.empty()) {
                        cpp << "namespace " << m_namespace;
                        cpp << "{";
                        ops = boost::in_place(std::ref(cpp));
                    }

                    // ctor
                    writeCtorImplementations(cpp, c);

                    // dtor
                    writeDtorImplementation(cpp, c);

                    // parent pointers
                    writeParentPointerGetterImplementation(cpp, c);

                    // uid manager
                    writeUidManagerGetterImplementation(cpp, c);

                    // io
                    writeReadImplementation(cpp, c, c.fields);
                    writeWriteImplementation(cpp, c, c.fields);

                    // choice validator
                    writeChoiceValidatorImplementation(cpp, c);

                    // accessors
                    writeAccessorImplementations(cpp, c.name, c.fields);

                    // tree manipulators
                    writeTreeManipulatorImplementations(cpp, c);

                    if (hasUidRefField(c))
                        writeUidRefObjectFunctionImplementations(cpp, c);

                    if (!m_namespace.empty()) {
                        ops = boost::none;
                        cpp << "}";
                    }
                }
                cpp << "} // namespace generated";
            }
            cpp << "} // namespace tigl";
            cpp << EmptyLine;
        }

        void writeClass(IndentingStreamWrapper& hpp, IndentingStreamWrapper& cpp, const Class& c) const {
            const auto includes = resolveIncludes(c);
            writeHeader(hpp, c, includes);
            writeSource(cpp, c, includes);
        }

        auto enumCppName(std::string name, const Tables& tables) const {
            // prefix numbers with "num" and replace minus with "neg"
            if (std::isdigit(name[0]))
                name = "_" + name;
            if (name[0] == '-' && name.size() > 1 && std::isdigit(name[1]))
                name = "_neg" + name.substr(1);

            // replace some chars which are not allowed in C++
            std::replace_if(std::begin(name), std::end(name), [](char c) {
                return !std::isalnum(c);
            }, '_');

            // prefix reserved identifiers
            if (tables.m_reservedNames.contains(name))
                name = "_" + name;

            return name;
        }

        void writeEnum(IndentingStreamWrapper& hpp, const Enum& e) const {
            // file header
            writeLicenseHeader(hpp);

            hpp << "#pragma once";
            hpp << EmptyLine;

            // includes
            hpp << "#include <string>";
            if (!c_generateCaseSensitiveStringToEnumConversion)
                hpp << "#include <cctype>";
            hpp << EmptyLine;
            hpp << "#include \"CTiglError.h\"";
            hpp << "#include \"to_string.h\"";
            hpp << EmptyLine;

            // namespace
            hpp << "namespace tigl";
            hpp << "{";
            {

                hpp << "namespace generated";
                hpp << "{";
                {
                    Scope s(hpp);

                    boost::optional<Scope> ops;
                    if (!m_namespace.empty()) {
                        hpp << "namespace " << m_namespace;
                        hpp << "{";
                        ops = boost::in_place(std::ref(hpp));
                    }

                    // meta information from schema
                    hpp << "// This enum is used in:";
                    for (const auto& c : e.deps.parents) {
                        hpp << "// " << c->name << "";
                    }
                    hpp << EmptyLine;

                    // enum name
                    hpp << "enum " << (c_generateCpp11ScopedEnums ? "class " : "") << e.name;
                    hpp << "{";
                    {
                        Scope s(hpp);

                        // values
                        for (const auto& v : e.values)
                            hpp << enumCppName(v.name(), m_tables) << (&v != &e.values.back() ? "," : "") << "";
                    }
                    hpp << "};";
                    hpp << EmptyLine;


                    // enum to string function
                    const auto& prefix = c_generateCpp11ScopedEnums ? e.name + "::" : "";
                    hpp << "inline std::string " << enumToStringFunc(e, m_tables) << "(const " << e.name << "& value)";
                    hpp << "{";
                    {
                        Scope s(hpp);
                        hpp << "switch(value) {";
                        {
                            //Scope s(hpp);
                            for (const auto& v : e.values)
                                hpp << "case " << prefix << enumCppName(v.name(), m_tables) << ": return \"" << v.cpacsName << "\";";
                            hpp << "default: throw CTiglError(\"Invalid enum value \\\"\" + std_to_string(static_cast<int>(value)) + \"\\\" for enum type " << e.name << "\");";
                        }
                        hpp << "}";
                    }
                    hpp << "}";

                    // string to enum function
                    hpp << "inline " << e.name << " " << stringToEnumFunc(e, m_tables) << "(const std::string& value)";
                    hpp << "{";
                    {
                        Scope s(hpp);
                        if (c_generateCaseSensitiveStringToEnumConversion) {
                            for (const auto& v : e.values)
                                hpp << "if (value == \"" << v.cpacsName << "\") return " << prefix << enumCppName(v.name(), m_tables) << ";";
                        } else {
                            hpp << "auto toLower = [](std::string str) { for (char& c : str) { c = std::tolower(c); } return str; };";
                            auto toLower = [](std::string str) { for (char& c : str) c = std::tolower(c); return str; };
                            for (const auto& v : e.values)
                                hpp << "if (toLower(value) == \"" << toLower(v.cpacsName) << "\") { return " << prefix << enumCppName(v.name(), m_tables) << "; }";
                        }

                        hpp << "throw CTiglError(\"Invalid string value \\\"\" + value + \"\\\" for enum type " << e.name << "\");";
                    }

                    hpp << "}";

                    if (!m_namespace.empty()) {
                        ops = boost::none;
                        hpp << "}";
                    }
                }
                hpp << "} // namespace generated";
                hpp << EmptyLine;

                // export non-custom types into tigl namespace
                const auto generatedNs = "generated" + (m_namespace.empty() ? "" : "::" + m_namespace);

                const auto& customName = m_tables.m_customTypes.find(e.name);
                if (customName) {
                    hpp << "// " << e.name << " is customized, use type " << *customName << " directly";
                } else {
                    hpp << "// Aliases in tigl namespace";

                    boost::optional<Scope> ops;
                    if (!m_namespace.empty()) {
                        hpp << "namespace " << m_namespace;
                        hpp << "{";
                        ops = boost::in_place(std::ref(hpp));
                    }
                    hpp << "using E" << e.name << " = " << generatedNs << "::" << e.name << ";";
                    if (!m_namespace.empty()) {
                        ops = boost::none;
                        hpp << "}";
                    }
                }
            }
            hpp << "} // namespace tigl";
            hpp << EmptyLine;
        }
    };

    void genCode(const std::string& outputLocation, TypeSystem typeSystem, const std::string& ns, const Tables& tables, Filesystem& fs) {
        CodeGen gen(std::move(typeSystem), ns, tables);
        gen.writeFiles(outputLocation, fs);
    }
}

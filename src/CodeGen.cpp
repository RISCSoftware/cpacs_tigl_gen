#include <vector>
#include <cctype>
#include <fstream>
#include <algorithm>
#include <sstream>
#include <iomanip>

#include <boost/algorithm/string.hpp>
#include "SchemaParser.h"
#include "Tables.h"
#include "TypeSystem.h"
#include "NotImplementedException.h"
#include "IndentingStreamWrapper.h"
#include "WriteIfDifferentFiles.h"
#include "CodeGen.h"

namespace tigl {
    // some options
    namespace {
        const auto c_generateDefaultCtorsForParentPointerTypes = false;
        const auto c_generateCaseSensitiveStringToEnumConversion = false; // true would be more strict, but some test data has troubles with this
        const auto c_generateTryCatchAroundOptionalClassReads = true;
        const auto c_generateCpp11ScopedEnums = false;

        const auto tixiHelperNamespace = "tixihelper";
        const auto c_uidMgrName = std::string("CTiglUIDManager");
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

        auto hasUidField(const Class& c) {
            for (const auto& f : c.fields)
                if (f.name() == "uID")
                    return true;
            return false;
        }

        auto hasMandatoryUidField(const Class& c) {
            for (const auto& f : c.fields)
                if (f.name() == "uID" && f.cardinality == Cardinality::Mandatory)
                    return true;
            return false;
        }

        // TODO: this call recurses down the tree, if this gets too slow, we can traverse the tree once and mark all classes
        auto selfOrAnyChildHasUidField(const Class& c) {
            if (hasUidField(c))
                return true;
            for (const auto& cc : c.deps.children)
                if (selfOrAnyChildHasUidField(*cc))
                    return true;
            return false;
        }
    }

    class CodeGen {
    public:
        CodeGen(TypeSystem types, const Tables& tables)
            : m_types(std::move(types)), m_tables(tables) {}

        void writeFiles(const std::string& outputLocation) {
            WriteIfDifferentFiles files;

            for (const auto& p : m_types.classes) {
                const auto c = p.second;
                const auto hppFileName = outputLocation + "/" + c.name + ".h";
                const auto cppFileName = outputLocation + "/" + c.name + ".cpp";
                if (c.pruned) {
                    files.removeIfExists(hppFileName.c_str());
                    files.removeIfExists(cppFileName.c_str());
                    continue;
                }

                auto hpp = files.newFile(hppFileName);
                auto cpp = files.newFile(cppFileName);
                writeClass(IndentingStreamWrapper(hpp.stream()), IndentingStreamWrapper(cpp.stream()), c);
            }

            for (const auto& p : m_types.enums) {
                const auto e = p.second;
                const auto hppFileName = outputLocation + "/" + e.name + ".h";
                if (e.pruned) {
                    files.removeIfExists(hppFileName.c_str());
                    continue;
                }

                auto hpp = files.newFile(hppFileName);
                writeEnum(IndentingStreamWrapper(hpp.stream()), e);
            }

            std::cout << "\tWrote   " << std::setw(5) << files.newlywritten << " new files" << std::endl;
            std::cout << "\tUpdated " << std::setw(5) << files.overwritten << " existing files" << std::endl;
            std::cout << "\tSkipped " << std::setw(5) << files.skipped << " files, no changes" << std::endl;
            std::cout << "\tDeleted " << std::setw(5) << files.deleted << " files, pruned" << std::endl;
        }

    private:
        struct Includes {
            std::vector<std::string> hppIncludes;
            std::vector<std::string> hppForwards;
            std::vector<std::string> hppCustomForwards;
            std::vector<std::string> cppIncludes;
        };

        TypeSystem m_types;
        const Tables&     m_tables;

        auto customReplacedType(const std::string& name) const -> std::string {
            return tigl::customReplacedType(name, m_tables);
        }

        auto customReplacedType(const Field& field) const -> std::string {
            return customReplacedType(field.typeName);
        }

        auto getterSetterType(const Field& field) const -> std::string {
            const auto typeName = customReplacedType(field);
            switch (field.cardinality) {
                case Cardinality::Optional:
                    return "boost::optional<" + typeName + ">";
                case Cardinality::Mandatory:
                    return typeName;
                case Cardinality::Vector:
                {
                    if (m_types.classes.find(field.typeName) != std::end(m_types.classes))
                        return "std::vector<unique_ptr<" + typeName + "> >";
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

        void writeFields(IndentingStreamWrapper& hpp, const std::vector<Field>& fields) {
            std::size_t length = 0;
            for (const auto& f : fields)
                length = std::max(length, fieldType(f).length());

            for (const auto& f : fields) {
                //hpp << "// generated from " << f.originXPath;
                hpp << std::left << std::setw(length) << fieldType(f) << " " << f.fieldName() << ";";

                //if (&f != &fields.back())
                //    hpp << "";
            }
            if (fields.size() > 0)
                hpp << "";
        }

        void writeAccessorDeclarations(IndentingStreamWrapper& hpp, const std::vector<Field>& fields) {
            for (const auto& f : fields) {
                hpp << "TIGL_EXPORT virtual const " << getterSetterType(f) << "& Get" << capitalizeFirstLetter(f.name()) << "() const;";

                // generate setter only for fundamental and enum types which are not vectors
                const bool isClassType = m_types.classes.find(f.typeName) != std::end(m_types.classes);
                if (!isClassType && f.cardinality != Cardinality::Vector) {
                    if (f.cardinality == Cardinality::Optional)
                        hpp << "TIGL_EXPORT virtual void Set" << capitalizeFirstLetter(f.name()) << "(const " << customReplacedType(f) << "& value);";

                    hpp << "TIGL_EXPORT virtual void Set" << capitalizeFirstLetter(f.name()) << "(const " << getterSetterType(f) << "& value);";
                } else
                    hpp << "TIGL_EXPORT virtual " << getterSetterType(f) << "& Get" << capitalizeFirstLetter(f.name()) << "();";
                hpp << "";
            }
        }

        void writeAccessorImplementations(IndentingStreamWrapper& cpp, const std::string& className, const std::vector<Field>& fields) {
            for (const auto& f : fields) {
                cpp << "const " << getterSetterType(f) << "& " << className << "::Get" << capitalizeFirstLetter(f.name()) << "() const";
                cpp << "{";
                {
                    Scope s(cpp);
                    cpp << "return " << f.fieldName() << ";";
                }
                cpp << "}";
                cpp << "";

                // generate setter only for fundamental and enum types which are not vectors
                const bool isClassType = m_types.classes.find(f.typeName) != std::end(m_types.classes);
                if (!isClassType && f.cardinality != Cardinality::Vector) {
                    auto writeUidRegistration = [&](bool memberOp, bool argOp) {
                        if (f.name() == "uID") {
                            cpp << "if (m_uidMgr) {";
                            {
                                Scope s(cpp);
                                if (memberOp)
                                    cpp << "if (m_uID) m_uidMgr->UnregisterObject(*m_uID);";
                                else
                                    cpp << "m_uidMgr->UnregisterObject(m_uID);";

                                if (argOp)
                                    cpp << "if (value) m_uidMgr->RegisterObject(*value, *this);";
                                else
                                    cpp << "m_uidMgr->RegisterObject(value, *this);";
                            }
                            cpp << "}";
                        }
                    };
                    const auto isOptional = f.cardinality == Cardinality::Optional;
                    if (isOptional) {
                        cpp << "void " << className << "::Set" << capitalizeFirstLetter(f.name()) << "(const " << customReplacedType(f) << "& value)";
                        cpp << "{";
                        {
                            Scope s(cpp);
                            writeUidRegistration(isOptional, false);
                            cpp << f.fieldName() << " = value;";
                        }
                        cpp << "}";
                        cpp << "";
                    }

                    cpp << "void " << className << "::Set" << capitalizeFirstLetter(f.name()) << "(const " << getterSetterType(f) << "& value)";
                    cpp << "{";
                    {
                        Scope s(cpp);
                        writeUidRegistration(isOptional, isOptional);
                        cpp << f.fieldName() << " = value;";
                    }
                    cpp << "}";
                } else {
                    cpp << getterSetterType(f) << "& " << className << "::Get" << capitalizeFirstLetter(f.name()) << "()";
                    cpp << "{";
                    {
                        Scope s(cpp);
                        cpp << "return " << f.fieldName() << ";";
                    }
                    cpp << "}";
                }
                cpp << "";
            }
        }

        void writeParentPointerGetters(IndentingStreamWrapper& hpp, const Class& c) {
            if (m_tables.m_parentPointers.contains(c.name)) {
                if (c.deps.parents.size() > 1) {
                    hpp << "template<typename P>";
                    hpp << "TIGL_EXPORT bool IsParent() const";
                    hpp << "{";
                    {
                        Scope s(hpp);
                        hpp << "return m_parentType != NULL && *m_parentType == typeid(P);";
                    }
                    hpp << "}";
                    hpp << "";
                    hpp << "template<typename P>";
                    hpp << "TIGL_EXPORT P* GetParent() const";
                    hpp << "{";
                    {
                        Scope s(hpp);
                        hpp << "#ifdef HAVE_STDIS_SAME";
                        hpp << "static_assert(";
                        for (const auto& dep : c.deps.parents) {
                            if (&dep != &c.deps.parents[0])
                                hpp.raw() << " || ";
                            hpp.raw() << "std::is_same<P, " << customReplacedType(dep->name) << ">::value";
                        }
                        hpp.raw() << ", \"template argument for P is not a parent class of " << c.name << "\");";
                        hpp << "#endif";
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
                } else if (c.deps.parents.size() == 1) {
                    hpp << "TIGL_EXPORT " << customReplacedType(c.deps.parents[0]->name) << "* GetParent() const;";
                }
                hpp << "";
            }
        }

        void writeParentPointerGetterImplementation(IndentingStreamWrapper& cpp, const Class& c) {
            if (m_tables.m_parentPointers.contains(c.name)) {
                if (c.deps.parents.size() == 1) {
                    cpp << customReplacedType(c.deps.parents[0]->name) << "* " << c.name << "::GetParent() const";
                    cpp << "{";
                    {
                        Scope s(cpp);
                        cpp << "return m_parent;";
                    }
                    cpp << "}";
                    cpp << "";
                }
            }
        }

        void writeIODeclarations(IndentingStreamWrapper& hpp, const std::string& className, const std::vector<Field>& fields) {
            hpp << "TIGL_EXPORT virtual void ReadCPACS(const TixiDocumentHandle& tixiHandle, const std::string& xpath);";
            hpp << "TIGL_EXPORT virtual void WriteCPACS(const TixiDocumentHandle& tixiHandle, const std::string& xpath) const;";
            hpp << "";
        }

        auto isAttribute(const XMLConstruct& construct) -> bool {
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

        auto xmlConstructToString(const XMLConstruct& construct) -> std::string {
            switch (construct) {
                case XMLConstruct::Attribute:           return "attribute";
                case XMLConstruct::Element:             return "element";
                case XMLConstruct::SimpleContent:       return "simpleContent";
                case XMLConstruct::FundamentalTypeBase: return "fundamental type base class";
                default: throw std::logic_error("Unknown XML construct");
            }
        }

        auto ctorArgumentList(const Class& c, const Class& parentClass) -> std::string {
            std::vector<std::string> arguments;
            const auto requiresParentPointer = m_tables.m_parentPointers.contains(c.name);
            if (requiresParentPointer)
                arguments.push_back(parentPointerThis(parentClass));
            const auto requiresUidManager = selfOrAnyChildHasUidField(c);
            if (requiresUidManager)
                arguments.push_back("m_uidMgr");
            return boost::join(arguments, ", ");
        }

        void writeReadAttributeOrElementImplementation(IndentingStreamWrapper& cpp, const Class& c, const Field& f) {
            const bool isAtt = isAttribute(f.xmlType);

            // fundamental types
            if (m_tables.m_fundamentalTypes.contains(f.typeName)) {
                switch (f.cardinality) {
                    case Cardinality::Optional:
                    case Cardinality::Mandatory:
                        if (isAtt)
                            cpp << f.fieldName() << " = " << tixiHelperNamespace << "::TixiGetAttribute<" << f.typeName << ">(tixiHandle, xpath, \"" + f.cpacsName + "\");";
                        else {
                            const auto empty = f.xmlType == XMLConstruct::SimpleContent || f.xmlType == XMLConstruct::FundamentalTypeBase;
                            cpp << f.fieldName() << " = " << tixiHelperNamespace << "::TixiGetElement<" << f.typeName << ">(tixiHandle, xpath" << (empty ? "" : " + \"/" + f.cpacsName + "\"") << ");";
                        }
                        break;
                    case Cardinality::Vector:
                        if (f.xmlType == XMLConstruct::Attribute || f.xmlType == XMLConstruct::SimpleContent || f.xmlType == XMLConstruct::FundamentalTypeBase)
                            throw std::runtime_error("Attributes, simpleContents and bases cannot be vectors");
                        assert(!isAtt);
                        cpp << tixiHelperNamespace << "::TixiReadElements(tixiHandle, xpath + \"/" << f.cpacsName << "\", " << f.fieldName() << ");";
                        break;
                }

                return;
            }

            // enums
            const auto itE = m_types.enums.find(f.typeName);
            if (itE != std::end(m_types.enums)) {
                const auto& readFunc = stringToEnumFunc(itE->second, m_tables);
                switch (f.cardinality) {
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
            if (f.xmlType != XMLConstruct::Attribute && f.xmlType != XMLConstruct::FundamentalTypeBase) {
                const auto itC = m_types.classes.find(f.typeName);
                const auto requiresParentPointer = m_tables.m_parentPointers.contains(f.typeName);
                if (itC != std::end(m_types.classes)) {
                    switch (f.cardinality) {
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
                                cpp << "} catch(const CTiglError& e) {";
                                {
                                    Scope s(cpp);
                                    cpp << "LOG(ERROR) << \"Failed to read " << f.cpacsName << " at xpath \" << xpath << \": \" << e.getError();";
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
                            cpp << tixiHelperNamespace << "::TixiReadElements(tixiHandle, xpath + \"/" << f.cpacsName << "\", " << f.fieldName() << (moreArgs.empty() ? "" : ", " + moreArgs) << ");";
                            break;
                    }
                    return;
                }
            }

            throw std::logic_error("No read function provided for type " + f.typeName);
        }

        void writeWriteAttributeOrElementImplementation(IndentingStreamWrapper& cpp, const Field& f) {
            const auto isAtt = isAttribute(f.xmlType);
            const auto empty = f.xmlType == XMLConstruct::SimpleContent || f.xmlType == XMLConstruct::FundamentalTypeBase;

            auto createElement = [&] {
                if (!empty)
                    cpp << tixiHelperNamespace << "::TixiCreateElementIfNotExists(tixiHandle, xpath + \"/" + f.cpacsName + "\");";
            };

            // fundamental types
            if (m_tables.m_fundamentalTypes.contains(f.typeName)) {
                switch (f.cardinality) {
                    case Cardinality::Optional:
                        cpp << "if (" << f.fieldName() << ") {";
                        {
                            Scope s(cpp);
                            createElement();
                            if (isAtt)
                                cpp << tixiHelperNamespace << "::TixiSaveAttribute(tixiHandle, xpath, \"" + f.cpacsName + "\", *" << f.fieldName() << ");";
                            else
                                cpp << tixiHelperNamespace << "::TixiSaveElement(tixiHandle, xpath" << (empty ? "" : " + \"/" + f.cpacsName + "\"") << ", *" << f.fieldName() << ");";
                        }
                        cpp << "}";
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
                switch (f.cardinality) {
                    case Cardinality::Optional:
                        cpp << "if (" << f.fieldName() << ") {";
                        {
                            Scope s(cpp);
                            createElement();
                            cpp << tixiHelperNamespace << "::TixiSave" << (isAtt ? "Attribute" : "Element") << "(tixiHandle, xpath" << (isAtt ? ", \"" : " + \"/") << f.cpacsName + "\", " << enumToStringFunc(itE->second, m_tables) << "(*" << f.fieldName() << "));";
                        }
                        cpp << "}";
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
                    switch (f.cardinality) {
                        case Cardinality::Optional:
                            cpp << "if (" << f.fieldName() << ") {";
                            {
                                Scope s(cpp);
                                createElement();
                                cpp << f.fieldName() << "->WriteCPACS(tixiHandle, xpath + \"/" << f.cpacsName << "\");";
                            }
                            cpp << "}";
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

        void writeReadBaseImplementation(IndentingStreamWrapper& cpp, const std::string& type) {
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

        void writeWriteBaseImplementation(IndentingStreamWrapper& cpp, const std::string& type) {
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

        void writeReadImplementation(IndentingStreamWrapper& cpp, const Class& c, const std::vector<Field>& fields) {
            cpp << "void " << c.name << "::ReadCPACS(const TixiDocumentHandle& tixiHandle, const std::string& xpath)";
            cpp << "{";
            {
                Scope s(cpp);

                // base class
                if (!c.base.empty()) {
                    cpp << "// read base";
                    writeReadBaseImplementation(cpp, c.base);
                    cpp << "";
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
                    if (f.cardinality == Cardinality::Mandatory) {
                        // attribute or element must exist
                        cpp << "else {";
                        {
                            Scope s(cpp);
                            cpp << "LOG(ERROR) << \"Required " << construct << " " << f.cpacsName << " is missing at xpath \" << xpath;";
                        }
                        cpp << "}";
                    }
                    cpp << "";
                }

                // register
                if (hasUidField(c)) {
                    if (hasMandatoryUidField(c))
                        cpp << "if (m_uidMgr) m_uidMgr->RegisterObject(m_uID, *this);";
                    else
                        cpp << "if (m_uidMgr && m_uID) m_uidMgr->RegisterObject(*m_uID, *this);";
                }
            }
            cpp << "}";
            cpp << "";
        }

        void writeWriteImplementation(IndentingStreamWrapper& cpp, const Class& c, const std::vector<Field>& fields) {
            cpp << "void " << c.name << "::WriteCPACS(const TixiDocumentHandle& tixiHandle, const std::string& xpath) const";
            cpp << "{";
            {
                Scope s(cpp);

                // base class
                if (!c.base.empty()) {
                    cpp << "// write base";
                    writeWriteBaseImplementation(cpp, c.base);
                    cpp << "";
                }

                // fields
                for (const auto& f : fields) {
                    const auto construct = xmlConstructToString(f.xmlType);
                    cpp << "// write " << construct << " " << f.cpacsName;
                    writeWriteAttributeOrElementImplementation(cpp, f);
                    cpp << "";
                }
            }
            cpp << "}";
            cpp << "";
        }

        void writeLicenseHeader(IndentingStreamWrapper& f) {
            f.raw() << "// Copyright (c) 2016 RISC Software GmbH";
            f       << "//";
            f       << "// This file was generated by CPACSGen from CPACS XML Schema (c) German Aerospace Center (DLR/SC).";
            f       << "// Do not edit, all changes are lost when files are re-generated.";
            f       << "//";
            f       << "// Licensed under the Apache License, Version 2.0 (the \"License\")";
            f       << "// you may not use this file except in compliance with the License.";
            f       << "// You may obtain a copy of the License at";
            f       << "//";
            f       << "//     http://www.apache.org/licenses/LICENSE-2.0";
            f       << "//";
            f       << "// Unless required by applicable law or agreed to in writing, software";
            f       << "// distributed under the License is distributed on an \"AS IS\" BASIS,";
            f       << "// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.";
            f       << "// See the License for the specific language governing permissions and";
            f       << "// limitations under the License.";
            f       << "";
        }

        auto resolveIncludes(const Class& c) -> Includes {
            Includes deps;

            deps.hppIncludes.push_back("<tixi.h>");
            deps.hppIncludes.push_back("<string>");
            deps.hppIncludes.push_back("\"tigl_internal.h\"");

            // optional, vector and make_unique
            bool vectorHeader = false;
            bool makeUnique = false;
            bool optionalHeader = false;
            bool timeHeader = false;
            for (const auto& f : c.fields) {
                switch (f.cardinality) {
                    case Cardinality::Optional:
                        optionalHeader = true;
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
            }
            if (timeHeader)
                deps.hppIncludes.push_back("<ctime>");
            if (c.deps.parents.size() > 1) {
                deps.hppIncludes.push_back("\"CTiglError.h\"");
                deps.hppIncludes.push_back("<typeinfo>");
            }
            if (selfOrAnyChildHasUidField(c)) {
                deps.hppCustomForwards.push_back(c_uidMgrName);
                deps.cppIncludes.push_back("\"" + c_uidMgrName + ".h\"");
            }

            // base class
            if (!c.base.empty() && m_types.classes.find(c.base) != std::end(m_types.classes)) {
                if (auto p = m_tables.m_customTypes.find(c.base))
                    deps.hppIncludes.push_back("<" + *p + ".h>");
                else
                    deps.hppIncludes.push_back("\"" + c.base + ".h\"");
            }

            // fields
            for (const auto& f : c.fields) {
                if (m_types.enums.find(f.typeName) != std::end(m_types.enums) ||
                    m_types.classes.find(f.typeName) != std::end(m_types.classes)) {
                    // this is a class or enum type, include it

                    const auto p = m_tables.m_customTypes.find(f.typeName);
                    if (!p) {
                        switch (f.cardinality) {
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
                        switch (f.cardinality) {
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
            if (m_tables.m_parentPointers.contains(c.name)) {
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

        void writeCtors(IndentingStreamWrapper& hpp, const Class& c) {
            const auto hasUid = selfOrAnyChildHasUidField(c);
            if (m_tables.m_parentPointers.contains(c.name)) {
                if (c_generateDefaultCtorsForParentPointerTypes)
                    hpp << "TIGL_EXPORT " << c.name << "(" << (hasUid ? c_uidMgrName + "* uidMgr" : "") << ");";
                for (const auto& dep : c.deps.parents)
                    hpp << "TIGL_EXPORT " << c.name << "(" << customReplacedType(dep->name) << "* parent" << (hasUid ? ", " + c_uidMgrName + "* uidMgr" : "") << ");";
                hpp << "";
            } else {
                hpp << "TIGL_EXPORT " << c.name << "(" << (hasUid ? c_uidMgrName + "* uidMgr" : "") << ");";
            }
        }

        void writeParentPointerAndUidManagerFields(IndentingStreamWrapper& hpp, const Class& c) {
            if (m_tables.m_parentPointers.contains(c.name)) {
                if (c.deps.parents.size() > 1) {
                    hpp << "void* m_parent;";
                    hpp << "const std::type_info* m_parentType;";
                } else if (c.deps.parents.size() == 1) {
                    hpp << customReplacedType(c.deps.parents[0]->name) << "* m_parent;";
                }
                hpp << "";
            }
            if (selfOrAnyChildHasUidField(c)) {
                hpp << c_uidMgrName << "* m_uidMgr;";
                hpp << "";
            }
        }

        auto parentPointerThis(const Class& c) const -> std::string {
            const auto cust = m_tables.m_customTypes.find(c.name);
            if (cust)
                return "reinterpret_cast<" + *cust + "*>(this)";
            else
                return "this";
        }

        void writeCtorImplementations(IndentingStreamWrapper& cpp, const Class& c) {
            const auto hasUid = selfOrAnyChildHasUidField(c);

            auto writeInitializationList = [&] {
                Scope s(cpp);
                bool first = true;
                auto writeMember = [&](const std::string& memberName, const std::string& value) {
                    if (first) {
                        cpp.raw() << " :";
                        first = false;
                    } else
                        cpp.raw() << ", ";
                    cpp << memberName << "(" << value << ")";
                };
                if (hasUid)
                    writeMember("m_uidMgr", "uidMgr");
                for (const auto& f : c.fields) {
                    if (f.cardinality == Cardinality::Mandatory) {
                        const auto fci = m_types.classes.find(f.typeName);
                        if (fci != std::end(m_types.classes)) {
                            const auto args = ctorArgumentList(fci->second, c);
                            if(!args.empty())
                                writeMember(f.fieldName(), args);
                        }
                    }
                }
            };

            // if this class holds parent pointers, we have to provide corresponding ctor overloads
            if (m_tables.m_parentPointers.contains(c.name)) {
                if (c_generateDefaultCtorsForParentPointerTypes) {
                    cpp << c.name << "::" << c.name << "(" << (hasUid ? c_uidMgrName + "* uidMgr" : "") << ")";
                    writeInitializationList();
                    cpp << "{";
                    {
                        Scope s(cpp);
                        cpp << "m_parent = NULL;";
                        if (c.deps.parents.size() > 1)
                            cpp << "m_parentType = NULL;";
                    }
                    cpp << "}";
                    cpp << "";
                }
                for (const auto& dep : c.deps.parents) {
                    const auto rn = customReplacedType(dep->name);
                    cpp << c.name << "::" << c.name << "(" << rn << "* parent" << (hasUid ? ", " + c_uidMgrName + "* uidMgr" : "") << ")";
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
                    cpp << "";
                }
            } else {
                cpp << c.name << "::" << c.name << "(" << (hasUid ? c_uidMgrName + "* uidMgr" : "") << ")";
                writeInitializationList();
                cpp.raw() << " {}";
                cpp << "";
            }
        }

        void writeDtor(IndentingStreamWrapper& hpp, const Class& c) {
            hpp << "TIGL_EXPORT virtual ~" << c.name << "();";
            hpp << "";
        }

        void writeDtorImplementation(IndentingStreamWrapper& cpp, const Class& c) {
            if (hasUidField(c)) {
                cpp << c.name << "::~" << c.name << "()";
                cpp << "{";
                {
                    Scope s(cpp);
                    if (hasMandatoryUidField(c))
                        cpp << "if (m_uidMgr) m_uidMgr->UnregisterObject(m_uID);";
                    else
                        cpp << "if (m_uidMgr && m_uID) m_uidMgr->UnregisterObject(*m_uID);";
                }
                cpp << "}";
            } else
                cpp << c.name << "::~" << c.name << "() {}";
            cpp << "";
        }

        void writeHeader(IndentingStreamWrapper& hpp, const Class& c, const Includes& includes) {
            // file header
            writeLicenseHeader(hpp);

            hpp << "#pragma once";
            hpp << "";

            // includes
            for (const auto& inc : includes.hppIncludes)
                hpp << "#include " << inc << "";
            if (includes.hppIncludes.size() > 0)
                hpp << "";

            // namespace
            hpp << "namespace tigl";
            hpp << "{";
            {
                Scope s(hpp);

                // custom Tigl types declarations
                for (const auto& fwd : includes.hppCustomForwards)
                    hpp << "class " << fwd << ";";
                if (includes.hppCustomForwards.size() > 0)
                    hpp << "";

                hpp << "namespace generated";
                hpp << "{";
                {
                    Scope s(hpp);

                    // forward declarations
                    for (const auto& fwd : includes.hppForwards)
                        hpp << "class " << fwd << ";";
                    if (includes.hppForwards.size() > 0)
                        hpp << "";

                    // meta information from schema
                    hpp << "// This class is used in:";
                    for (const auto& p : c.deps.parents)
                        hpp << "// " << p->name << "";
                    if (c.deps.parents.size() > 0)
                        hpp << "";

                    hpp << "// generated from " << c.originXPath << "";

                    // class name and base class
                    hpp << "class " << c.name << (c.base.empty() ? "" : " : public " + customReplacedType(c.base));
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

                        // io
                        writeIODeclarations(hpp, c.name, c.fields);

                        // accessors
                        writeAccessorDeclarations(hpp, c.fields);

                    }
                    hpp << "protected:";
                    {
                        Scope s(hpp);

                        // parent pointers and uid manager
                        writeParentPointerAndUidManagerFields(hpp, c);

                        // fields
                        writeFields(hpp, c.fields);
                    }
                    hpp << "private:";
                    {
                        Scope s(hpp);

                        // copy ctor and assign, move ctor and assign
                        hpp << "#ifdef HAVE_CPP11";
                        hpp << c.name << "(const " << c.name << "&) = delete;";
                        hpp << "" << c.name << "& operator=(const " << c.name << "&) = delete;";
                        hpp << "";
                        hpp << c.name << "(" << c.name << "&&) = delete;";
                        hpp << c.name << "& operator=(" << c.name << "&&) = delete;";
                        hpp << "#else";
                        hpp << c.name << "(const " << c.name << "&);";
                        hpp << c.name << "& operator=(const " << c.name << "&);";
                        hpp << "#endif";
                    }
                    hpp << "};";
                }
                hpp << "}";
                hpp << "";

                // export non-custom types into tigl namespace
                std::vector<std::string> exportedTypes;
                const auto& customName = m_tables.m_customTypes.find(c.name);
                if (customName) {
                    hpp << "// " << c.name << " is customized, use type " << *customName << " directly";
                    if (includes.hppForwards.size() > 0)
                        hpp << "";
                } else
                    exportedTypes.push_back(c.name);

                for (const auto& fwd : includes.hppForwards)
                    exportedTypes.push_back(fwd);

                if (!exportedTypes.empty()) {
                    hpp << "// Aliases in tigl namespace";
                    hpp << "#ifdef HAVE_CPP11";
                    for (const auto& name : exportedTypes)
                        hpp << "using C" << name << " = generated::" << name << ";";
                    hpp << "#else";
                    for (const auto& name : exportedTypes)
                        hpp << "typedef generated::" << name << " C" << name << ";";
                    hpp << "#endif";
                }
            }
            hpp << "}";
            hpp << "";
        }

        void writeSource(IndentingStreamWrapper& cpp, const Class& c, const Includes& includes) {
            // file header
            writeLicenseHeader(cpp);

            // includes
            for (const auto& inc : includes.cppIncludes)
                cpp << "#include " << inc << "";
            if (includes.cppIncludes.size() > 0)
                cpp << "";

            // namespace
            cpp << "namespace tigl";
            cpp << "{";
            {
                Scope s(cpp);

                cpp << "namespace generated";
                cpp << "{";
                {
                    Scope s(cpp);

                    // ctor
                    writeCtorImplementations(cpp, c);

                    // dtor
                    writeDtorImplementation(cpp, c);

                    // parent pointers
                    writeParentPointerGetterImplementation(cpp, c);

                    // io
                    writeReadImplementation(cpp, c, c.fields);
                    writeWriteImplementation(cpp, c, c.fields);

                    // accessors
                    writeAccessorImplementations(cpp, c.name, c.fields);
                }
                cpp << "}";
            }
            cpp << "}";
            cpp << "";
        }

        void writeClass(IndentingStreamWrapper& hpp, IndentingStreamWrapper& cpp, const Class& c) {
            const auto includes = resolveIncludes(c);
            writeHeader(hpp, c, includes);
            writeSource(cpp, c, includes);
        }

        auto enumCppName(std::string name, const Tables& tables) {
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

        void writeEnum(IndentingStreamWrapper& hpp, const Enum& e) {
            // file header
            writeLicenseHeader(hpp);

            hpp << "#pragma once";
            hpp << "";

            // includes
            hpp << "#include <string>";
            if (!c_generateCaseSensitiveStringToEnumConversion)
                hpp << "#include <cctype>";
            hpp << "";
            hpp << "#include \"CTiglError.h\"";
            hpp << "#include \"to_string.h\"";
            hpp << "";

            // namespace
            hpp << "namespace tigl";
            hpp << "{";
            {
                Scope s(hpp);

                hpp << "namespace generated";
                hpp << "{";
                {
                    Scope s(hpp);

                    // meta information from schema
                    hpp << "// This enum is used in:";
                    for (const auto& c : e.deps.parents) {
                        hpp << "// " << c->name << "";
                    }
                    hpp << "";
                    hpp << "// generated from " << e.originXPath;

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
                    hpp << "";


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
                            if (c_generateCpp11ScopedEnums)
                                hpp << "auto toLower = [](std::string str) { for (char& c : str) { c = std::tolower(c); } return str; };";
                            else
                                hpp << "struct ToLower { std::string operator()(std::string str) { for (std::size_t i = 0; i < str.length(); i++) { str[i] = std::tolower(str[i]); } return str; } } toLower;";
                            auto toLower = [](std::string str) { for (char& c : str) c = std::tolower(c); return str; };
                            for (const auto& v : e.values)
                                hpp << "if (toLower(value) == \"" << toLower(v.cpacsName) << "\") { return " << prefix << enumCppName(v.name(), m_tables) << "; }";
                        }

                        hpp << "throw CTiglError(\"Invalid string value \\\"\" + value + \"\\\" for enum type " << e.name << "\");";
                    }
                    hpp << "}";
                }
                hpp << "}";
                hpp << "";

                // export non-custom types into tigl namespace
                const auto& customName = m_tables.m_customTypes.find(e.name);
                if (customName) {
                    hpp << "// " << e.name << " is customized, use type " << *customName << " directly";
                } else {
                    hpp << "// Aliases in tigl namespace";
                    if (!c_generateCpp11ScopedEnums)
                        hpp << "#ifdef HAVE_CPP11";
                    hpp << "using E" << e.name << " = generated::" << e.name << ";";
                    if (!c_generateCpp11ScopedEnums) {
                        hpp << "#else";
                        hpp << "typedef generated::" << e.name << " E" << e.name << ";";
                        hpp << "#endif";
                        for (const auto& v : e.values)
                            hpp << "using generated::" << enumCppName(v.name(), m_tables) << ";";
                    }
                }
            }
            hpp << "}";
            hpp << "";
        }
    };

    void genCode(const std::string& outputLocation, TypeSystem typeSystem, const Tables& tables) {
        CodeGen gen(std::move(typeSystem), tables);
        gen.writeFiles(outputLocation);
    }
}

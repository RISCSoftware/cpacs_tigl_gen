#include <fstream>
#include <sstream>
#include <iostream>

#include "Tables.h"

namespace tigl {
    Table::Table(std::unordered_set<std::string> data)
        : m_set(std::move(data)) {}

    Table::Table(const std::string& filename) {
        read(filename);
    }

    void Table::read(const std::string& filename) {
        std::ifstream f(filename);
        if (!f) {
            std::cerr << "Warning: Failed to open file " << filename << " for reading. No table data loaded" << std::endl;
            return;
        }

        std::cout << "Reading table " << filename << std::endl;

        std::string line;
        while (std::getline(f, line)) {
            // skip empty lines and comments
            if (line.empty() || line.compare(0, 2, "//") == 0)
                continue;

            // skip spaces and read one type name
            std::istringstream ss(line);
            std::string name;
            ss >> name;

            std::cout << "\t" << name << std::endl;
            m_set.insert(name);
        }
    }

    bool Table::contains(const std::string& key) const {
        return m_set.find(key) != std::end(m_set);
    }

    MappingTable::MappingTable(std::unordered_map<std::string, std::string> data)
        : m_map(std::move(data)) {}

    MappingTable::MappingTable(const std::string& filename) {
        read(filename);
    }

    void MappingTable::read(const std::string& filename) {
        std::ifstream f(filename);
        if (!f) {
            std::cerr << "Warning: Failed to open file " << filename << " for reading. No table data loaded" << std::endl;
            return;
        }

        std::cout << "Reading mapping table " << filename << std::endl;

        std::string line;
        while (std::getline(f, line)) {
            // skip empty lines and comments
            if (line.empty() || line.compare(0, 2, "//") == 0)
                continue;

            // skip spaces and read two type names
            std::istringstream ss(line);
            std::string first;
            std::string second;
            ss >> first >> second;

            std::cout << "\t" << first << " -> " << second << std::endl;
            m_map.insert(std::make_pair(first, second));
        }
    }

    bool MappingTable::contains(const std::string& key) const {
        return find(key).is_initialized();
    }

    boost::optional<const std::string&> MappingTable::find(const std::string& key) const {
        const auto it = m_map.find(key);
        if (it != std::end(m_map))
            return it->second;
        else
            return {};
    }

    void MappingTable::substituteIfExists(const std::string& key, std::string& value) const {
        const auto p = find(key);
        if (p)
            value = *p;
    }


    Tables::Tables(const std::string& inputDirectory) :
        m_customTypes      (inputDirectory + "/CustomTypes.txt"     ),
        m_typeSubstitutions(inputDirectory + "/TypeSubstitution.txt"),

        m_xsdTypes({
            { "xsd:byte"          , "int8_t" },
            { "xsd:unsignedByte"  , "uint8_t" },
            { "xsd:short"         , "int16_t" },
            { "xsd:unsignedShort" , "uint16_t" },
            { "xsd:int"           , "int32_t" },
            { "xsd:unsignedInt"   , "uint32_t" },
            { "xsd:long"          , "int64_t" },
            { "xsd:unsignedLong"  , "uint64_t" },
            { "xsd:integer"       , "int" },
            { "xsd:boolean"       , "bool" },
            { "xsd:float"         , "float" },
            { "xsd:double"        , "double" },
            { "xsd:decimal"       , "double" }, // TODO: implement custom type?
            { "xsd:date"          , "std::time_t" },
            { "xsd:dateTime"      , "std::time_t" },
            { "xsd:time"          , "std::time_t" },
            { "xsd:string"        , "std::string" },
            { "xsd:ID"            , "std::string" },
            { "xsd:IDREF"         , "std::string" }
        }),

        m_pruneList        (inputDirectory + "/PruneList.txt"       ),
        m_parentPointers   (inputDirectory + "/ParentPointer.txt"   ),

        m_reservedNames({
            // names reserved by C++
            // from: http://en.cppreference.com/w/cpp/keyword
            "alignas",
            "alignof",
            "and",
            "and_eq",
            "asm",
            "atomic_cancel",
            "atomic_commit",
            "atomic_noexcept",
            "auto",
            "bitand",
            "bitor",
            "bool",
            "break",
            "case",
            "catch",
            "char",
            "char16_t",
            "char32_t",
            "class",
            "compl",
            "concept",
            "const",
            "constexpr",
            "const_cast",
            "continue",
            "decltype",
            "default",
            "delete",
            "do",
            "double",
            "dynamic_cast",
            "else",
            "enum",
            "explicit",
            "export",
            "extern",
            "false",
            "float",
            "for",
            "friend",
            "goto",
            "if",
            "inline",
            "int",
            "import",
            "long",
            "module",
            "mutable",
            "namespace",
            "new",
            "noexcept",
            "not",
            "not_eq",
            "nullptr",
            "operator",
            "or",
            "or_eq",
            "private",
            "protected",
            "public",
            "register",
            "reinterpret_cast",
            "requires",
            "return",
            "short",
            "signed",
            "sizeof",
            "static",
            "static_assert",
            "static_cast",
            "struct",
            "switch",
            "synchronized",
            "template",
            "this",
            "thread_local",
            "throw",
            "true",
            "try",
            "typedef",
            "typeid",
            "typename",
            "union",
            "unsigned",
            "using",
            "virtual",
            "void",
            "volatile",
            "wchar_t",
            "while",
            "xor",
            "xor_eq"
        }),
        m_fundamentalTypes({
            "std::string",
            "double",
            "bool",
            "int",
            "std::time_t"
        }
        
        ) {}
}

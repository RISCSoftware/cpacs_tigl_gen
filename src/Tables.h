#pragma once

#include <boost/optional.hpp>
#include <string>
#include <unordered_map>
#include <unordered_set>

namespace tigl {
    class Table {
    public:
        Table(std::unordered_set<std::string> data);
        Table(const std::string& filename);

        bool contains(const std::string& key) const;

    private:
        void read(const std::string& filename);

        std::unordered_set<std::string> m_set;
    };

    class MappingTable {
    public:
        MappingTable(std::unordered_map<std::string, std::string> data);
        MappingTable(const std::string& filename);

        bool contains(const std::string& key) const;
        boost::optional<const std::string&> find(const std::string& key) const;
        void substituteIfExists(const std::string& key, std::string& value) const;

    private:
        void read(const std::string& filename);

        std::unordered_map<std::string, std::string> m_map;
    };

    class Tables {
    public:
        Tables(const std::string& inputDirectory);

        MappingTable m_customTypes;
        MappingTable m_typeSubstitutions;
        MappingTable m_xsdTypes;

        Table m_pruneList;
        Table m_parentPointers;
        Table m_reservedNames;
        Table m_fundamentalTypes;
    };
}

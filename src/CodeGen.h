#pragma once

#include <string>
#include <algorithm>
#include <stdexcept>
#include <unordered_map>

#include "Tables.h"
#include "Variant.hpp"
#include "SchemaParser.h"

namespace tigl {
	enum class Cardinality {
		Optional,
		Mandatory,
		Vector
	};

	inline std::string toString(const Cardinality& c) {
		switch (c) {
			case Cardinality::Optional: return "Optional";
			case Cardinality::Mandatory: return "Mandatory";
			case Cardinality::Vector: return "Vector";
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
		Variant<const Attribute*, const Element*, const SimpleContent*> origin;
		std::string cpacsName;
		std::string typeName;
		//Variant<Class*, Enum*> type;
		XMLConstruct xmlType;
		Cardinality cardinality;

		std::string customFieldName;

		auto name() const {
			if (!customFieldName.empty())
				return customFieldName;
			else
				return cpacsName;
		}

		auto fieldName() const {
			return "m_" + name();
		}
	};

	struct ClassDependencies {
		// non owning
		std::vector<Class*> bases;
		std::vector<Class*> deriveds;
		std::vector<Class*> parents;
		std::vector<Class*> children;
		std::vector<Enum*> enumChildren;
	};

	struct Class {
		const ComplexType* origin;
		std::string name;
		std::string base;
		std::vector<Field> fields;

		ClassDependencies deps;
	};

	struct EnumValue {
		std::string name;
		std::string cppName;

		EnumValue() = default;
		EnumValue(const std::string& name)
			: name(name) {
			cppName = name;

			// prefix numbers with "num" and replace minus with "neg"
			if (std::isdigit(cppName[0]))
				cppName = "_" + cppName;
			if (cppName[0] == '-' && cppName.size() > 1 && std::isdigit(cppName[1]))
				cppName = "_neg" + cppName.substr(1);

			// replace some chars which are not allowed in C++ for cppName
			std::replace_if(std::begin(cppName), std::end(cppName), [](char c) {
				return !std::isalnum(c);
			}, '_');

			// prefix reserved identifiers
			if (s_reservedNames.contains(cppName))
				cppName = "_" + cppName;
		}

		friend bool operator==(const EnumValue& a, const EnumValue& b) {
			return a.name == b.name;
		}
	};

	struct EnumDependencies {
		std::vector<Class*> parents;
	};

	struct Enum {
		const SimpleType* origin;
		std::string name;
		std::vector<EnumValue> values;

		EnumDependencies deps;
	};

	struct Types {
		std::unordered_map<std::string, Class> classes;
		std::unordered_map<std::string, Enum> enums;

		void buildTypeSystem();
	};

	class IndentingStreamWrapper;

	class CodeGen {
	public:
		CodeGen(const std::string& outputLocation, Types types);
	private:
		struct Includes {
			std::vector<std::string> hppIncludes;
			std::vector<std::string> hppForwards;
			std::vector<std::string> hppCustomForwards;
			std::vector<std::string> cppIncludes;
		};

		Types m_types;

		std::string getterSetterType(const Field& field) const;
		std::string fieldType(const Field& field) const;
		std::string parentPointerThis(const Class& c) const;

		void writeFields(IndentingStreamWrapper& hpp, const std::vector<Field>& fields);
		void writeAccessorDeclarations(IndentingStreamWrapper& hpp, const std::vector<Field>& fields);
		void writeAccessorImplementations(IndentingStreamWrapper& cpp, const std::string& className, const std::vector<Field>& fields);
		void writeParentPointerGetters(IndentingStreamWrapper& hpp, const Class& c);
		void writeParentPointerGetterImplementation(IndentingStreamWrapper& cpp, const Class& c);
		void writeIODeclarations(IndentingStreamWrapper& hpp, const std::string& className, const std::vector<Field>& fields);
		void writeReadAttributeOrElementImplementation(IndentingStreamWrapper& cpp, const Class& c, const Field& f);
		void writeWriteAttributeOrElementImplementation(IndentingStreamWrapper& cpp, const Field& f);
		void writeReadBaseImplementation(IndentingStreamWrapper& cpp, const std::string& type);
		void writeWriteBaseImplementation(IndentingStreamWrapper& cpp, const std::string& type);
		void writeReadImplementation(IndentingStreamWrapper& cpp, const Class& className, const std::vector<Field>& fields);
		void writeWriteImplementation(IndentingStreamWrapper& cpp, const Class& className, const std::vector<Field>& fields);
		void writeLicenseHeader(IndentingStreamWrapper& f);
		Includes resolveIncludes(const Class& c);
		void writeCtors(IndentingStreamWrapper& hpp, const Class& c);
		void writeParentPointerFields(IndentingStreamWrapper& hpp, const Class& c);
		void writeCtorImplementations(IndentingStreamWrapper& cpp, const Class& c);
		void writeHeader(IndentingStreamWrapper& hpp, const Class& c, const Includes& includes);
		void writeSource(IndentingStreamWrapper& cpp, const Class& c, const Includes& includes);
		void writeClass(IndentingStreamWrapper& hpp, IndentingStreamWrapper& cpp, const Class& c);
		void writeEnum(IndentingStreamWrapper& hpp, const Enum& e);
	};
}

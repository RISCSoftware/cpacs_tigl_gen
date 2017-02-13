#include <boost/filesystem.hpp>
#include <iostream>

#include "SchemaParser.h"
#include "TypeSystem.h"
#include "CodeGen.h"
#include "Tables.h"

namespace tigl {
    void run(const std::string& inputDirectory, const std::string& srcDirectory, const std::string& outputDirectory) {
        // load tables
        Tables tables(inputDirectory);

        // read types and elements
        const auto& cpacsLocation = inputDirectory + "/cpacs_schema.xsd";
        std::cout << "Parsing " << cpacsLocation << std::endl;
        SchemaParser schema(cpacsLocation);

        // generate type system from schema
        TypeSystem typeSystem(schema, tables);

        // build dependencies
        typeSystem.buildDependencies();

        // apply a few reductions and cleanups
        typeSystem.collapseEnums();

        typeSystem.runPruneList();

        // create output directory
        boost::filesystem::create_directories(outputDirectory);

        // generate code
        std::cout << "Generating classes" << std::endl;
        CodeGen codegen(outputDirectory, typeSystem, tables);

        std::cout << "Copying runtime" << std::endl;
        for (const auto& file : {
            "TixiDocument.h",
            "TixiHelper.h",
            "TixiHelper.cpp",
            "Optional.hpp",
        }) {
            std::cout << file << std::endl;
            boost::filesystem::copy_file(srcDirectory + "/" + file, outputDirectory + "/" + file, boost::filesystem::copy_option::none);
        }
    }
}

int main(int argc, char* argv[]) {
    // parse command line arguments
    if (argc != 4) {
        std::cerr << "Usage: CPACSGen inputDirectory srcDirectory outputDirectory" << std::endl;
        return -1;
    }

    const std::string inputDirectory  = argv[1];
    const std::string srcDirectory    = argv[2];
    const std::string outputDirectory = argv[3];

    try {
        tigl::run(inputDirectory, srcDirectory, outputDirectory);
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "Exception: " << e.what() << std::endl;
        return -1;
    } catch (...) {
        std::cerr << "Unknown exception" << std::endl;
        return -1;
    }
}

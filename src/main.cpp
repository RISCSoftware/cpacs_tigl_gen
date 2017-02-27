#include <boost/filesystem.hpp>
#include <iostream>
#include <iomanip>

#include "SchemaParser.h"
#include "TypeSystem.h"
#include "CodeGen.h"
#include "Tables.h"
#include "WriteIfDifferentFiles.h"

namespace tigl {
    const auto runtimeFiles = {
        "TixiHelper.h",
        "TixiHelper.cpp",
        "UniquePtr.h",
    };

    void run(const std::string& inputDirectory, const std::string& srcDirectory, const std::string& outputDirectory) {
        // load tables
        Tables tables(inputDirectory);

        // read types and elements
        const auto& cpacsLocation = inputDirectory + "/cpacs_schema.xsd";
        std::cout << "Parsing " << cpacsLocation << std::endl;
        SchemaParser schema(cpacsLocation);

        // generate type system from schema
        const auto& typeSystem = buildTypeSystem(schema, tables);

        // create output directory
        boost::filesystem::create_directories(outputDirectory);

        // generate code
        std::cout << "Generating classes" << std::endl;
        genCode(outputDirectory, typeSystem, tables);

        std::cout << "Copying runtime" << std::endl;
        WriteIfDifferentFiles files;
        for (const auto& file : runtimeFiles)
            files.newFile(outputDirectory + "/" + file).stream() << readFile(srcDirectory + "/" + file);
        std::cout << "\tWrote   " << std::setw(5) << files.newlywritten << " new files" << std::endl;
        std::cout << "\tUpdated " << std::setw(5) << files.overwritten << " existing files" << std::endl;
        std::cout << "\tSkipped " << std::setw(5) << files.skipped << " files, no changes" << std::endl;
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

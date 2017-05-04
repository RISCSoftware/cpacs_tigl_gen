#include <boost/filesystem.hpp>
#include <iostream>
#include <iomanip>

#include "SchemaParser.h"
#include "TypeSystem.h"
#include "CodeGen.h"
#include "Tables.h"
#include "WriteIfDifferentFiles.h"

namespace fs = boost::filesystem;

namespace tigl {
    const auto runtimeFiles = {
        "TixiHelper.h",
        "TixiHelper.cpp",
        "UniquePtr.h",
    };

    void run(const std::string& inputDirectory, const std::string& srcDirectory, const std::string& outputDirectory, const std::string& typeSystemGraphVisFile) {
        // load tables
        Tables tables(inputDirectory);

        // read types and elements
        const auto& cpacsLocation = inputDirectory + "/cpacs_schema.xsd";
        std::cout << "Parsing " << cpacsLocation << std::endl;
        auto types = parseSchema(cpacsLocation);

        // generate type system from schema
        std::cout << "Creating type system" << std::endl;
        const auto& typeSystem = buildTypeSystem(types, tables);

        // write graph vis file for the generated type system
        if (!typeSystemGraphVisFile.empty()) {
            auto p = fs::path{typeSystemGraphVisFile};
            if (p.has_parent_path())
                fs::create_directories(p.parent_path());
            writeGraphVisFile(typeSystem, typeSystemGraphVisFile);
        }

        // create output directory
        fs::create_directories(outputDirectory);

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
    if (argc != 4 && argc != 5) {
        std::cerr << "Usage: CPACSGen configDir cpascGenSourceDir outputDir [typeSystemGraphVisFile] \n\n";
        std::cerr << "Options:\n\n"
                     "  configDir          The directory containing the CPACS schema and\n"
                     "                     the table files.\n"
                     "  cpacsGenSourceDir  The src directory of the CPACSGen source code.\n"
                     "  outputDir          The directory to which the CPACSGen output\n"
                     "                     file are written\n"
                  << std::endl;
        return -1;
    }

    const std::string inputDirectory         = argv[1];
    const std::string srcDirectory           = argv[2];
    const std::string outputDirectory        = argv[3];
    const std::string typeSystemGraphVisFile = argc > 4 ? argv[4] : "";

    try {
        tigl::run(inputDirectory, srcDirectory, outputDirectory, typeSystemGraphVisFile);
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "Exception: " << e.what() << std::endl;
        return -1;
    } catch (...) {
        std::cerr << "Unknown exception" << std::endl;
        return -1;
    }
}

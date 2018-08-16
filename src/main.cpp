#include <boost/filesystem.hpp>
#include <iostream>
#include <iomanip>

#include "SchemaParser.h"
#include "TypeSystem.h"
#include "CodeGen.h"
#include "Tables.h"
#include "Filesystem.h"
#include "NotImplementedException.h"

namespace fs = boost::filesystem;

namespace tigl {
    const auto runtimeFiles = {
        "TixiHelper.h",
        "UniquePtr.h",
    };

    void processDirectory(const std::string& inputDirectory, const std::string& srcDirectory, const std::string& outputDirectory, const std::string& typeSystemGraphVisFile, Filesystem& fs, const std::string& ns = "") {
        // load tables from this directory
        const Tables tables(inputDirectory);

        // iterate all *.xsd files in the input directory
        for (const auto& e : fs::directory_iterator(inputDirectory)) {
            if (fs::is_regular_file(e) && e.path().has_extension() && e.path().extension() == ".xsd") {
                // read types and elements
                std::cout << "Parsing " << e.path() << std::endl;
                auto types = xsd::parseSchema(e.path().string());

                // generate type system from schema
                std::cout << "Creating type system" << std::endl;
                const auto& typeSystem = buildTypeSystem(types, tables);

                // write graph vis file for the generated type system
                if (!typeSystemGraphVisFile.empty()) {
                    auto p = fs::path{ typeSystemGraphVisFile };
                    if (p.has_parent_path())
                        fs::create_directories(p.parent_path());
                    writeGraphVisFile(typeSystem, typeSystemGraphVisFile);
                }

                // create output directory
                const auto nsOutputDirectory = ns.empty() ? outputDirectory : outputDirectory + "/" + ns;
                fs::create_directories(nsOutputDirectory);

                // generate code
                std::cout << "Generating classes" << std::endl;
                genCode(nsOutputDirectory, typeSystem, ns, tables, fs);
            }
        }

        // recurse on sub directories
        for (const auto& e : fs::directory_iterator(inputDirectory)) {
            if (fs::is_directory(e)) {
                if (!ns.empty())
                    throw NotImplementedException("Nested input directories are not implemented. Only 1 level of subdirectories (namespaces) is allowed.");

                const auto& leafDir = e.path().leaf().string();
                processDirectory(e.path().string(), srcDirectory, outputDirectory, typeSystemGraphVisFile, fs, leafDir);
            }
        }
    }

    void run(const std::string& inputDirectory, const std::string& srcDirectory, const std::string& outputDirectory, const std::string& typeSystemGraphVisFile) {
        // create runtime output directory
        fs::create_directories(outputDirectory);

        std::cout << "Copying runtime" << std::endl;
        Filesystem fs;
        for (const auto& file : runtimeFiles)
            fs.newFile(outputDirectory + "/" + file).stream() << readFile(srcDirectory + "/" + file);

        // process schema files
        processDirectory(inputDirectory, srcDirectory, outputDirectory, typeSystemGraphVisFile, fs);

        std::cout << "\tWrote   " << std::setw(5) << fs.newlywritten << " new files" << std::endl;
        std::cout << "\tUpdated " << std::setw(5) << fs.overwritten << " existing files" << std::endl;
        std::cout << "\tSkipped " << std::setw(5) << fs.skipped << " files, no changes" << std::endl;
        std::cout << "\tDeleted " << std::setw(5) << fs.deleted << " files, pruned" << std::endl;
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

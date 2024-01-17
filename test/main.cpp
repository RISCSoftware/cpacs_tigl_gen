#define BOOST_TEST_MODULE CPACSGenTests
#include <boost/test/unit_test.hpp>

#include "../src/lib/SchemaParser.h"
#include "../src/lib/TypeSystem.h"
#include "../src/lib/CodeGen.h"
#include "../src/lib/Tables.h"
#include "../src/lib/Filesystem.h"

#include "utils.h"

void runTest() {
    const auto testDir = ::testDir();
    const auto schemaFile = testDir / "schema.xsd";
    const auto refFile = testDir / "ref.cpp";
    const auto resultFile = testDir / "result.cpp";

    tigl::Filesystem fs;

    const tigl::Tables tables(testDir.string());
    auto types = tigl::xsd::parseSchema(schemaFile.string());
    const auto& typeSystem = tigl::buildTypeSystem(types, tables);
    genCode(testDir.string(), typeSystem, "", tables, fs);

    fs.mergeFilesInto(resultFile);
    fs.flushToDisk();

    const auto ref = readTextFile(refFile);
    const auto result = readTextFile(resultFile);
    if (ref != result)
        BOOST_TEST_ERROR("ref and result mismatch. please diff files in filesystem");
    else
        boost::filesystem::remove(resultFile);
}

BOOST_AUTO_TEST_CASE(sequence) {
    runTest();
}

BOOST_AUTO_TEST_CASE(all) {
    runTest();
}

BOOST_AUTO_TEST_CASE(choice) {
    runTest();
}

BOOST_AUTO_TEST_CASE(documentation) {
    runTest();
}

BOOST_AUTO_TEST_CASE(uidinbasetype) {
    runTest();
}

BOOST_AUTO_TEST_CASE(custombasetype) {
    runTest();
}

BOOST_AUTO_TEST_CASE(basetypewithparent) {
    runTest();
}

BOOST_AUTO_TEST_CASE(uidreferencevector) {
    runTest();
}

BOOST_AUTO_TEST_CASE(cdata) {
    runTest();
}

BOOST_AUTO_TEST_CASE(simplebasetypewithparent) {
    runTest();
}

BOOST_AUTO_TEST_CASE(complextypewithsimplecontent) {
    runTest();
}

BOOST_AUTO_TEST_CASE(collapsedifferentenums) {
    runTest();
}

BOOST_AUTO_TEST_CASE(optionalchoice) {
    runTest();
}

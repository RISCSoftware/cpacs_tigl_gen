#pragma once

#include <boost/filesystem.hpp>
#include <boost/test/unit_test.hpp>

#include "paths.h"

auto readTextFile(const boost::filesystem::path& path) -> std::string;

auto testName() -> std::string;

auto testDir() -> boost::filesystem::path;

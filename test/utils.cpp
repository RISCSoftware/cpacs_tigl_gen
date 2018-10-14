#include "utils.h"

auto readTextFile(const boost::filesystem::path& path) -> std::string {
    std::ifstream f(path.string());
    if (!f)
        throw std::ios::failure("Failed to open file " + path.string() + " for reading");

    f.seekg(std::ios::end);
    const auto size = f.tellg();
    f.seekg(std::ios::beg);

    std::string content;
    content.reserve(size);
    content.assign(std::istreambuf_iterator<char>{f}, std::istreambuf_iterator<char>{});
    return content;
}

auto testName() -> std::string {
    return boost::unit_test::framework::current_test_case().p_name.get();
}

auto testDir() -> boost::filesystem::path {
    return boost::filesystem::path{ c_dataDir } / testName();
}

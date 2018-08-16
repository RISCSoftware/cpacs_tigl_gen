#pragma once

#include <boost/filesystem.hpp>
#include <string>
#include <sstream>
#include <memory>

namespace tigl {
    auto readFile(const boost::filesystem::path& filename) -> std::string;

    class Filesystem;

    class File {
    public:
        File(const File&) = delete;
        File& operator=(const File&) = delete;
        File(File&&) = default;
        File& operator=(File&&) = default;
        ~File();

        auto stream() -> std::ostream&;

    private:
        friend class Filesystem;

        File(boost::filesystem::path filename, Filesystem* parent);

        std::unique_ptr<std::stringstream> m_stream; // workaround for GCC < 5.0, where stringstream is not moveable ..
        boost::filesystem::path m_filename;
        Filesystem* m_parent;
    };

    class Filesystem {
    public:
        auto newFile(const boost::filesystem::path& filename) -> File;
        void removeIfExists(const boost::filesystem::path& path);

        std::size_t newlywritten = 0;
        std::size_t overwritten = 0;
        std::size_t skipped = 0;
        std::size_t deleted = 0;
    };
}

#pragma once

#include <boost/optional.hpp>
#include <boost/filesystem.hpp>
#include <string>
#include <sstream>
#include <memory>
#include <deque>

namespace tigl {
    auto readFile(const boost::filesystem::path& filename) -> std::string;

    class Filesystem;

    class File {
    public:
        File(boost::filesystem::path filename);
        File(const File&) = delete;
        File& operator=(const File&) = delete;
        File(File&&) = default;
        File& operator=(File&&) = default;

        auto path() const -> const boost::filesystem::path&;
        auto stream() -> std::ostream&;

    private:
        friend class Filesystem;

        std::unique_ptr<std::ostringstream> m_stream; // workaround for GCC < 5.0, where stringstream is not moveable ..
        boost::filesystem::path m_filename;
    };

    class Filesystem {
    public:
        Filesystem() = default;

        auto newFile(boost::filesystem::path filename) -> File&;
        void removeIfExists(const boost::filesystem::path& path);

        void mergeFilesInto(boost::filesystem::path filename);

        void flushToDisk();

        std::size_t newlywritten = 0;
        std::size_t overwritten = 0;
        std::size_t skipped = 0;
        std::size_t deleted = 0;

    private:
        void sortFiles();

        std::deque<File> m_files;
    };
}

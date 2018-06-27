#pragma once

#include <string>
#include <sstream>
#include <memory>

namespace tigl {
    auto readFile(const std::string& filename) -> std::string;

    class WriteIfDifferentFiles;

    class WriteIfDifferentFile {
    public:
        WriteIfDifferentFile(const WriteIfDifferentFile&) = delete;
        WriteIfDifferentFile& operator=(const WriteIfDifferentFile&) = delete;
        WriteIfDifferentFile(WriteIfDifferentFile&&) = default;
        WriteIfDifferentFile& operator=(WriteIfDifferentFile&&) = default;
        ~WriteIfDifferentFile();

        auto stream() -> std::ostream&;

    private:
        friend class WriteIfDifferentFiles;

        WriteIfDifferentFile(std::string filename, WriteIfDifferentFiles* parent);

        std::unique_ptr<std::stringstream> m_stream; // workaround for GCC < 5.0, where stringstream is not moveable ..
        std::string m_filename;
        WriteIfDifferentFiles* m_parent;
    };

    class WriteIfDifferentFiles {
    public:
        auto newFile(const std::string& filename) -> WriteIfDifferentFile;
        void removeIfExists(const std::string& path);

        std::size_t newlywritten = 0;
        std::size_t overwritten = 0;
        std::size_t skipped = 0;
        std::size_t deleted = 0;
    };
}

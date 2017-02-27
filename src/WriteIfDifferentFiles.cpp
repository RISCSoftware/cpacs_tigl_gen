#include "WriteIfDifferentFiles.h"

#include <fstream>

namespace tigl {
	namespace {
		auto fileExists(const std::string& filename) {
			std::ifstream f(filename);
			return f.good();
		}
	}

	auto readFile(const std::string& filename) -> std::string {
		std::ifstream existingFile(filename);
		if (!existingFile)
			return{};

		// read file to string
		existingFile.seekg(0, existingFile.end);
		const auto length = existingFile.tellg();
		existingFile.seekg(0, existingFile.beg);
		std::string content;
		content.reserve(length);
		content.assign(std::istreambuf_iterator<char>{existingFile}, std::istreambuf_iterator<char>{});
		return content;
	}

	WriteIfDifferentFile::WriteIfDifferentFile(const std::string& filename, WriteIfDifferentFiles* parent)
		: m_filename(filename), m_parent(parent) {}

	auto WriteIfDifferentFile::stream() -> std::ostream& {
		return m_stream;
	}

	WriteIfDifferentFile::~WriteIfDifferentFile() {
		const auto& newContent = m_stream.str();

		{
			// check if a file already exists
			if (fileExists(m_filename)) {
				// read existing file to string and compare
				const auto& content = readFile(m_filename);

				// if existing file has same content, skip overwriting it
				if (content == newContent) {
					m_parent->skipped++;
					return;
				}

				m_parent->overwritten++;
			} else
				m_parent->newlywritten++;
		}

		// write new content to file
		std::ofstream f(m_filename);
		f.exceptions(std::ios::failbit | std::ios::badbit);
		f.write(newContent.c_str(), newContent.size());
	}

	auto WriteIfDifferentFiles::newFile(const std::string& filename) -> WriteIfDifferentFile {
		return WriteIfDifferentFile(filename, this);
	}

	void WriteIfDifferentFiles::removeIfExists(const std::string & path) {
		if (fileExists(path)) {
			std::remove(path.c_str());
			deleted++;
		}
	}
}

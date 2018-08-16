#include "Filesystem.h"

#include <fstream>
#include <utility>

namespace tigl {
	auto readFile(const boost::filesystem::path& filename) -> std::string {
		std::ifstream existingFile(filename.string());
		if (!existingFile)
			return{};

		// read file to string
		existingFile.seekg(0, std::ifstream::end);
		const auto length = existingFile.tellg();
		existingFile.seekg(0, std::ifstream::beg);
		std::string content;
		content.reserve(length);
		content.assign(std::istreambuf_iterator<char>{existingFile}, std::istreambuf_iterator<char>{});
		return content;
	}

	File::File(boost::filesystem::path filename, Filesystem* parent)
		: m_stream(new std::stringstream), m_filename(std::move(filename)), m_parent(parent) {}

	auto File::stream() -> std::ostream& {
		return *m_stream;
	}

	File::~File() {
		const auto& newContent = m_stream->str();

		{
			// check if a file already exists
			if (boost::filesystem::exists(m_filename)) {
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
		std::ofstream f(m_filename.string());
		f.exceptions(std::ios::failbit | std::ios::badbit);
		f.write(newContent.c_str(), newContent.size());
	}

	auto Filesystem::newFile(const boost::filesystem::path& filename) -> File {
		return File(filename, this);
	}

	void Filesystem::removeIfExists(const boost::filesystem::path& path) {
		if (boost::filesystem::exists(path)) {
			boost::filesystem::remove(path);
			deleted++;
		}
	}
}

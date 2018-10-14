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

	File::File(boost::filesystem::path filename)
		: m_stream(new std::ostringstream), m_filename(std::move(filename)) {}

	auto File::stream() -> std::ostream& {
		return *m_stream;
	}

	void Filesystem::flushToDisk() {
		for (auto& file : m_files) {
			const auto& newContent = file.m_stream->str();

			// check if a file already exists
			if (boost::filesystem::exists(file.m_filename)) {
				// read existing file to string and compare
				const auto& content = readFile(file.m_filename);

				// if existing file has same content, skip overwriting it
				if (content == newContent) {
					skipped++;
					continue;
				}

				overwritten++;
			}
			else
				newlywritten++;

			// write new content to file
			std::ofstream f(file.m_filename.string());
			f.exceptions(std::ios::failbit | std::ios::badbit);
			f.write(newContent.c_str(), newContent.size());
			f.close();
		}
	}

	auto Filesystem::newFile(boost::filesystem::path filename) -> File& {
		m_files.emplace_back(std::move(filename));
		return m_files.back();
	}

	void Filesystem::removeIfExists(const boost::filesystem::path& path) {
		if (boost::filesystem::exists(path)) {
			boost::filesystem::remove(path);
			deleted++;
		}
	}

	void Filesystem::mergeFilesInto(boost::filesystem::path filename) {
		auto& f = newFile(std::move(filename));
		for (auto& file : m_files) {
			auto str = file.m_stream->str();
			f.m_stream->write(str.c_str(), str.size());
		}
	}
}

/**
 * \file lib/library/beatmaps.cc
 * \ingroup library_beatmaps
 */

#include "library/beatmaps.h"

#include "beatmap/beatmap.h"
#include "core/log.h"

#include <dirent.h>
#include <iostream>
#include <sstream>
#include <system_error>

namespace oshu {
namespace library {

/**
 * \todo
 * Implement.
 */
beatmap_entry::beatmap_entry(const std::string &path)
{
	oshu_beatmap beatmap;
	int rc = oshu_load_beatmap_headers(path.c_str(), &beatmap);
	if (rc < 0)
		throw std::runtime_error("could not load beatmap " + path);
	title = beatmap.metadata.title;
	artist = beatmap.metadata.artist;
	version = beatmap.metadata.version;
	oshu_destroy_beatmap(&beatmap);
}

std::ostream& operator<<(std::ostream &os, const beatmap_entry &entry)
{
	return os << entry.artist << " - " << entry.title << " [" << entry.version << "]";
}

static bool osu_file(const char *filename)
{
	size_t l = strlen(filename);
	if (l < 4)
		return false;
	return !strcmp(filename + l - 4, ".osu");
}

beatmap_set::beatmap_set(const std::string &path)
{
	DIR *dir = opendir(path.c_str());
	if (!dir)
		throw std::system_error(errno, std::system_category(), "could not open the beatmap set directory " + path);
	for (;;) {
		errno = 0;
		struct dirent* entry = readdir(dir);
		if (errno) {
			throw std::system_error(errno, std::system_category(), "could not read the beatmap set directory " + path);
		} else if (!entry) {
			// end of directory
			break;
		} else if (entry->d_name[0] == '.') {
			// hidden file, ignore
			continue;
		} else if (!osu_file(entry->d_name)) {
			continue;
		} else {
			try {
				std::ostringstream os;
				os << path << "/" << entry->d_name;
				entries.emplace_back(os.str());
			} catch(std::runtime_error&) {
				oshu::log::warning() << "ignoring invalid beatmap " << path << std::endl;
			}
		}
	}
}

/**
 * \todo
 * Factor it with #beatmap_set::beatmap_set.
 */
std::vector<beatmap_set> find_beatmap_sets(const std::string &path)
{
	std::vector<beatmap_set> sets;

	DIR *dir = opendir(path.c_str());
	if (!dir)
		throw std::system_error(errno, std::system_category(), "could not open the beatmaps directory");
	for (;;) {
		errno = 0;
		struct dirent* entry = readdir(dir);
		if (errno) {
			throw std::system_error(errno, std::system_category(), "could not read the beatmaps directory");
		} else if (!entry) {
			// end of directory
			break;
		} else if (entry->d_name[0] == '.') {
			// hidden directory, ignore
			continue;
		} else {
			try {
				std::ostringstream os;
				os << path << "/" << entry->d_name;
				sets.emplace_back(os.str());
			} catch (std::system_error& e) {
				oshu::log::debug() << e.what() << std::endl;
			}
		}
	}
	return sets;
}

}}

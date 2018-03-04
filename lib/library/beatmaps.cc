/**
 * \file lib/library/beatmaps.cc
 * \ingroup library_beatmaps
 */

#include "library/beatmaps.h"

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
	oshu::log::verbose() << "beatmap_entry(" << path << ")" << std::endl;
}

/**
 * \todo
 * Implement.
 */
beatmap_set::beatmap_set(const std::string &path)
{
	oshu::log::verbose() << "beatmap_set(" << path << ")" << std::endl;
}

std::vector<beatmap_set> find_beatmap_sets(const std::string &path)
{
	std::vector<beatmap_set> sets;

	DIR *root_dir = opendir(path.c_str());
	if (!root_dir)
		throw std::system_error(errno, std::system_category(), "could not open the beatmaps directory");

	for (;;) {
		errno = 0;
		struct dirent* entry = readdir(root_dir);
		if (errno) {
			throw std::system_error(errno, std::system_category(), "could not read the beatmaps directory");
		} else if (!entry) {
			// end of directory
			break;
		} else if (entry->d_name[0] == '.') {
			// hidden directory, ignore
			continue;
		} else {
			std::ostringstream os;
			os << path << "/" << entry->d_name;
			sets.emplace_back(os.str());
		}
	}

	return sets;
}

}}

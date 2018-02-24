/**
 * \file src/oshu-library/library.cc
 * \ingroup library
 */

#include <library/library.h>

#include <system_error>

using namespace oshu::library;

beatmap_finder::beatmap_finder()
{
	root_dir = opendir("beatmaps");
	if (!root_dir)
		throw std::system_error(errno, std::system_category(), "could not open the beatmaps directory");
}

beatmap_finder::~beatmap_finder()
{
	closedir(root_dir);
}

/**
 * \todo
 * Return an actual list of .osu files, not their directory.
 */
std::string beatmap_finder::next()
{
	for (;;) {
		errno = 0;
		struct dirent* entry = readdir(root_dir);
		if (errno)
			throw std::system_error(errno, std::system_category(), "could not read the beatmaps directory");
		else if (!entry)
			// end of directory
			return {};
		else if (entry->d_name[0] == '.')
			// hidden directory, ignore
			;
		else
			return std::string{entry->d_name};
	}
}

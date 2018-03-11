/**
 * \file src/oshu-library.cc
 */

#include "core/log.h"
#include "library/beatmaps.h"
#include "library/html.h"

#include <cstdlib>
#include <fstream>
#include <iostream>
#include <sys/stat.h>
#include <unistd.h>

static void ensure_directory(const std::string &path)
{
	if (mkdir(path.c_str(), S_IRWXU | S_IRWXG | S_IRWXO) < 0) {
		if (errno == EEXIST)
			return;
		throw std::system_error(errno, std::system_category(), "could not create directory " + path);
	} else {
		oshu::log::debug() << "created directory " << path << std::endl;
	}
}

static void change_directory(const std::string &path)
{
	if (chdir(path.c_str()) < 0)
		throw std::system_error(errno, std::system_category(), "could not chdir to " + path);
	else
		oshu::log::debug() << "moving to " << path << std::endl;
}

/**
 * Read the oshu! beatmap library location from the environment.
 *
 * By order of priority:
 *
 * 1. If OSHU_HOME is set, use it.
 * 2. If HOME is set, append `/.oshu/` at the end.
 * 3. Otherwise, throw an exception.
 *
 */
static std::string get_oshu_home()
{
	const char *home = std::getenv("OSHU_HOME");
	if (home && *home)
		return home;
	home = std::getenv("HOME");
	if (home && *home)
		return std::string(home) + "/.oshu";
	throw std::runtime_error("could not locate the oshu! home");
}

int main(int argc, char **argv)
{
	oshu::log::priority = oshu::log::level::verbose;
	std::string home = get_oshu_home();
	ensure_directory(home);
	ensure_directory(home + "/web");
	change_directory(home + "/web");
	auto sets = oshu::library::find_beatmap_sets("../beatmaps");
	std::ofstream index("index.html");
	oshu::library::html::generate_beatmap_set_listing(sets, index);
	return 0;
}

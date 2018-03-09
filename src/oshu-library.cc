/**
 * \file src/oshu-library/main.cc
 */

#include "core/log.h"
#include "library/beatmaps.h"
#include "library/html.h"

#include <cstdlib>
#include <fstream>
#include <iostream>
#include <unistd.h>

/**
 * Read the oshu! beatmap library location from the environment.
 *
 * By order of priority:
 *
 * 1. If OSHU_HOME is set, use it.
 * 2. If HOME is set, append `/.oshu/` at the end.
 * 3. Otherwise, return nothing.
 *
 */
static std::string get_oshu_home()
{
	const char *home = std::getenv("OSHU_HOME");
	if (home && *home)
		return home;
	home = std::getenv("HOME");
	if (home && *home)
		return std::string(home) + "/.oshu/";
	return {};
}

/**
 * Change the current working directory to oshu!'s home.
 *
 * If nothing is found, better exit the program rather than be too implicit and
 * perform unwanted things.
 */
static void move_home()
{
	std::string home = get_oshu_home();
	if (home.empty())
		throw std::runtime_error("could not locate the oshu! home");
	int rc = chdir(home.c_str());
	if (rc < 0)
		throw std::system_error(errno, std::system_category(), "could not chdir to " + home);
}

int main(int argc, char **argv)
{
	oshu::log::priority = oshu::log::level::verbose;
	move_home();
	auto sets = oshu::library::find_beatmap_sets("beatmaps");
	std::ofstream index("index.html");
	oshu::library::html::generate_beatmap_set_listing(sets, index);
	return 0;
}

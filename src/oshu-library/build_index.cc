/**
 * \file src/oshu-library/build_index.cc
 */

#include <cstdlib>
#include <fstream>
#include <getopt.h>
#include <iostream>
#include <sys/stat.h>
#include <unistd.h>

#include "core/log.h"
#include "library/beatmaps.h"
#include "library/html.h"

#include "./command.h"

enum option_values {
	OPT_VERBOSE = 'v',
};

static struct option options[] = {
	{"verbose", no_argument, 0, OPT_VERBOSE},
	{0, 0, 0, 0},
};

static const char *flags = "v";

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

static void do_build_index()
{
	std::string home = get_oshu_home();
	oshu::log::info() << "oshu! home directory: " << home << std::endl;
	ensure_directory(home);
	ensure_directory(home + "/web");
	change_directory(home + "/web");
	auto sets = oshu::library::find_beatmap_sets("../beatmaps");
	std::ofstream index("index.html");
	oshu::library::html::generate_beatmap_set_listing(sets, index);
	std::cout << home << "/web/index.html" << std::endl;
}

static int run(int argc, char **argv)
{
	for (;;) {
		int c = getopt_long(argc, argv, flags, options, NULL);
		if (c == -1)
			break;
		switch (c) {
		case OPT_VERBOSE:
			--oshu::log::priority;
			break;
		}
	}
	if (argc - optind != 0) {
		std::cerr << "Usage: oshu-library build-index [-v]" << std::endl;
		std::cerr << "       oshu-library --help" << std::endl;
		return 2;
	}
	SDL_LogSetAllPriority(SDL_LOG_PRIORITY_WARN);
	SDL_LogSetPriority(SDL_LOG_CATEGORY_APPLICATION, static_cast<SDL_LogPriority>(oshu::log::priority));
	do_build_index();
	return 0;
}

command build_index {
	.name = "build-index",
	.run = run,
};

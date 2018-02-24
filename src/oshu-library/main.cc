/**
 * \file src/oshu-library/main.cc
 * \ingroup library
 */

#include <library/library.h>

#include <cstdlib>
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

static void print_current_directory()
{
	char buffer[FILENAME_MAX];
	if (!getcwd(buffer, sizeof(buffer)))
		throw std::system_error(errno, std::system_category());
	std::cout << "Here: " << buffer << std::endl;
}

int main(int argc, char **argv)
{
	move_home();
	print_current_directory();
	std::string path;
	oshu::library::beatmap_finder finder;
	while (path = finder.next(), !path.empty()) {
		std::cout << path << std::endl;
	}
	return 0;
}

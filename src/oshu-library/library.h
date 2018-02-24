/**
 * \file src/oshu-library/library.h
 * \ingroup library
 */

#pragma once

#include <string>

#include <dirent.h>

/**
 * \defgroup library Library
 *
 * \brief
 * Manage a collection of beatmaps.
 *
 * Here is the expected directory structure of the library:
 *
 * ```
 * $OSHU_HOME
 * └── beatmaps
 *     └── 651934 Kaori Oda - Zero Tokei (Short ver.)
 *         ├── audio.mp3
 *         ├── bg1.jpg
 *         ├── Kaori Oda - Zero Tokei (Short ver.) (ShogunMoon) [Easy].osu
 *         ├── Kaori Oda - Zero Tokei (Short ver.) (ShogunMoon) [Normal].osu
 *         └── Kaori Oda - Zero Tokei (Short ver.) (ShogunMoon) [Shining].osu
 * ```
 *
 * \{
 */

namespace oshu {
namespace library {

/**
 * Walk the library directory and list all the beatmaps found.
 */
class beatmap_finder {
public:
	beatmap_finder();
	~beatmap_finder();
	/**
	 * Return the path of the found beatmap.
	 * When the end is reached, return an empty string.
	 */
	std::string next();
private:
	DIR* root_dir;
	DIR* beatmap_dir;
};

}}

/** \} */

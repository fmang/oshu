/**
 * \file include/library/beatmaps.h
 * \ingroup library_beatmaps
 */

#pragma once

#include "beatmap/beatmap.h"

#include <iosfwd>
#include <string>
#include <vector>

namespace oshu {

/**
 * \defgroup library_beatmaps Beatmaps
 * \ingroup library
 *
 * \brief
 * Manage a collection of beatmaps.
 *
 * Here is the expected directory structure of the beatmaps library:
 *
 * ```
 * root
 * └── 651934 Kaori Oda - Zero Tokei (Short ver.)
 *     ├── audio.mp3
 *     ├── bg1.jpg
 *     ├── Kaori Oda - Zero Tokei (Short ver.) (ShogunMoon) [Easy].osu
 *     ├── Kaori Oda - Zero Tokei (Short ver.) (ShogunMoon) [Normal].osu
 *     └── Kaori Oda - Zero Tokei (Short ver.) (ShogunMoon) [Shining].osu
 * ```
 *
 * The root above is the path you give to #find_beatmap_sets. Something like
 * `~/.oshu/beatmaps/`.
 *
 * The beatmap set is `651934 Kaori Oda - Zero Tokei (Short ver.)`, and
 * contains 3 beatmaps entry.
 *
 * \{
 */

/**
 * Gather the key information of a beatmap.
 *
 * To save resources, it uses #oshu_load_beatmap_headers.
 *
 * \todo
 * Reuse the beatmap structure?
 */
struct beatmap_entry {
	explicit beatmap_entry(const std::string &path);
	oshu_mode mode;
	/**
	 * Difficulty indicator.
	 *
	 * Naively, use the overall difficulty because there's nothing to
	 * compute. More advanced difficulty calculators exist but this would
	 * multiply the time required to build an index.
	 */
	int difficulty;
	std::string title;
	std::string artist;
	std::string version;
	/**
	 * Path to the .osu beatmap the entry was constructed with.
	 */
	std::string path;
};

/**
 * Group beatmaps from the same set together.
 *
 * In theory, all the beatmaps belonging to the same directory share the same
 * metadata and BeatmapSetID. Here, we'll just assume this is true and naively
 * list all the beatmaps found in the directory.
 *
 * \todo
 * Print a warning if the BeatmapSetID of beatmaps is inconsistent.
 */
struct beatmap_set {
	explicit beatmap_set(const std::string &path);
	/**
	 * List of beatmap entries inside this set, sorted by difficulty.
	 */
	std::vector<beatmap_entry> entries;
	bool empty() const;
	std::string title;
	std::string artist;
};

/**
 * Load all the entries found in the given directory.
 *
 * The entries are sorted alphabetically by artist, then by title.
 *
 * \warning
 * This function is expensive.
 *
 * \todo
 * Provide an iterator interface, if needed?
 */
std::vector<beatmap_set> find_beatmap_sets(const std::string &path);

/** } */

}

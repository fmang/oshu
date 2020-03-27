/**
 * \file lib/library/beatmaps.cc
 * \ingroup library_beatmaps
 */

#include "library/beatmaps.h"

#include "beatmap/beatmap.h"
#include "core/log.h"

#include <algorithm>
#include <dirent.h>
#include <iostream>
#include <sstream>
#include <system_error>

namespace oshu {

beatmap_entry::beatmap_entry(const std::string &path)
: path(path)
{
	oshu_beatmap beatmap;
	int rc = oshu_load_beatmap_headers(path.c_str(), &beatmap);
	if (rc < 0)
		throw std::runtime_error("could not load beatmap " + path);
	mode = beatmap.mode;
	difficulty = beatmap.difficulty.overall_difficulty;
	title = beatmap.metadata.title;
	artist = beatmap.metadata.artist;
	version = beatmap.metadata.version;
	oshu_destroy_beatmap(&beatmap);
}

static bool osu_file(const char *filename)
{
	size_t l = strlen(filename);
	if (l < 4)
		return false;
	return !strcmp(filename + l - 4, ".osu");
}

static void find_entries(const std::string &path, beatmap_set &set)
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
				beatmap_entry entry (os.str());
				if (entry.mode != OSHU_OSU_MODE)
					oshu::debug_log() << "skipping " << path << ": unsupported mode" << std::endl;
				else
					set.entries.push_back(std::move(entry));
			} catch(std::runtime_error &e) {
				oshu::warning_log() << e.what() << std::endl;
				oshu::warning_log() << "ignoring invalid beatmap " << path << std::endl;
			}
		}
	}
	closedir(dir);
}

static bool compare_entries(const beatmap_entry &a, const beatmap_entry &b)
{
	return a.difficulty < b.difficulty;
}

static bool compare_sets(const beatmap_set &a, const beatmap_set &b)
{
	int cmp = a.artist.compare(b.artist);
	if (cmp == 0) {
		// artist is the same, compare titles
		return a.title.compare(b.title) < 0;
	} else {
		return cmp < 0;
	}
}

beatmap_set::beatmap_set(const std::string &path)
{
	find_entries(path, *this);
	if (!empty()) {
		title = entries[0].title;
		artist = entries[0].artist;
		std::sort(entries.begin(), entries.end(), compare_entries);
	}
}

bool beatmap_set::empty() const
{
	return entries.empty();
};

std::vector<beatmap_set> find_beatmap_sets(const std::string &path)
{
	std::vector<beatmap_set> sets;

	DIR *dir = opendir(path.c_str());
	if (!dir)
		throw std::system_error(errno, std::system_category(), "could not open the beatmaps directory " + path);
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
				beatmap_set set (os.str());
				if (!set.empty())
					sets.push_back(std::move(set));
			} catch (std::system_error& e) {
				oshu::debug_log() << e.what() << std::endl;
			}
		}
	}
	closedir(dir);
	std::sort(sets.begin(), sets.end(), compare_sets);
	return sets;
}

}

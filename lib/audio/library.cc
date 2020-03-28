/**
 * \file audio/library.cc
 * \ingroup audio_library
 */

#include "config.h"

#include "audio/library.h"
#include "audio/audio.h"
#include "audio/sample.h"
#include "core/log.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <SDL2/SDL_timer.h>
#include <sstream>

/**
 * Determine the directory of the skin to use.
 *
 * See #oshu::sound_library::skin_directory for details.
 */
static std::string skin_directory()
{
	const char *env = getenv("OSHU_SKIN");
	if (!env || !*env) {
		;
	} else if (strchr(env, '/')) {
		if (!access(env, R_OK))
			return env; /* found! */
		else
			oshu_log_debug("could not find skin directory %s", env);
	} else {
		std::ostringstream os;
		os << OSHU_SKINS_DIRECTORY "/" << env;
		std::string dir = os.str();
		if (!access(dir.c_str(), R_OK))
			return dir;
		else
			oshu_log_debug("could not find skin %s in directory %s", env, OSHU_SKINS_DIRECTORY);
	}
	/* The OSHU_SKIN variable wasn't helpful, so let's resort to the default skin. */
	return OSHU_SKINS_DIRECTORY "/" OSHU_DEFAULT_SKIN;
}

void oshu::open_sound_library(struct oshu::sound_library *library, struct SDL_AudioSpec *format)
{
	library->skin_directory = skin_directory();
	oshu_log_debug("using skin directory %s", library->skin_directory.c_str());
	library->format = format;
}

static void free_sample(struct oshu::sample **sample)
{
	if (*sample) {
		oshu::destroy_sample(*sample);
		free(*sample);
		*sample = NULL;
	}
}

static void free_shelf(struct oshu::sound_shelf *shelf)
{
	free_sample(&shelf->hit_normal);
	free_sample(&shelf->hit_whistle);
	free_sample(&shelf->hit_finish);
	free_sample(&shelf->hit_clap);
	free_sample(&shelf->slider_slide);
	free_sample(&shelf->slider_whistle);
}

static void free_room(struct oshu::sound_room *room)
{
	if (room->shelves) {
		for (int i = 0; i < room->size; ++i)
			free_shelf(&room->shelves[i]);
		free(room->shelves);
		room->shelves = NULL;
	}
	free(room->indices);
	room->indices = NULL;
}

void oshu::close_sound_library(struct oshu::sound_library *library)
{
	free_room(&library->normal);
	free_room(&library->soft);
	free_room(&library->drum);
}

/**
 * Find a shelf in a room, looking for its index.
 *
 * Return NULL if the shelf wasn't found.
 */
static struct oshu::sound_shelf* find_shelf(struct oshu::sound_room *room, int index)
{
	for (int i = 0; i < room->size; ++i) {
		if (room->indices[i] == index)
			return &room->shelves[i];
	}
	return NULL;
}

/**
 * Allocate the space for a new shelf.
 *
 * If the capacity of the room is already big enough, there's nothing much to
 * do. However, if it's too small, the room is expanded by steps of 8 shelves.
 *
 * After calling this function, you are guaranteed that there is the capacity
 * for at least one new shelf (i.e. `room->size < room->capacity`).
 */
static void grow_room(struct oshu::sound_room *room)
{
	assert (room->size <= room->capacity);
	if (room->size < room->capacity)
		return;
	room->capacity += 8;
	room->shelves = (oshu::sound_shelf*) realloc(room->shelves, room->capacity * sizeof(*room->shelves));
	assert (room->shelves != NULL);
	room->indices = (int*) realloc(room->indices, room->capacity * sizeof(*room->indices));
	assert (room->indices != NULL);
}

/**
 * Add a shelf in a room with a specific index.
 *
 * Return the pointer to the new shelf.
 */
static struct oshu::sound_shelf* new_shelf(struct oshu::sound_room *room, int index)
{
	grow_room(room);
	int position = room->size++;
	room->indices[position] = index;
	memset(&room->shelves[position], 0, sizeof(*room->shelves));
	return &room->shelves[position];
}

/**
 * Get the room for a specified sample set.
 *
 * If the room wasnn't found, return NULL.
 */
static struct oshu::sound_room* get_room(struct oshu::sound_library *library, enum oshu::sample_set_family set)
{
	switch (set) {
	case oshu::NORMAL_SAMPLE_SET: return &library->normal;
	case oshu::SOFT_SAMPLE_SET:   return &library->soft;
	case oshu::DRUM_SAMPLE_SET:   return &library->drum;
	default:
		oshu_log_debug("unknown sample set %d", (int) set);
		return NULL;
	}
}

/**
 * Locate the sample on a shelf given its type.
 *
 * If the sample wasnn't found because its type doesn't exist, return NULL.
 */
static struct oshu::sample** get_sample(struct oshu::sound_shelf *shelf, int type)
{
	switch (type) {
	case oshu::HIT_SOUND|oshu::NORMAL_SOUND:  return &shelf->hit_normal;
	case oshu::HIT_SOUND|oshu::WHISTLE_SOUND: return &shelf->hit_whistle;
	case oshu::HIT_SOUND|oshu::FINISH_SOUND:  return &shelf->hit_finish;
	case oshu::HIT_SOUND|oshu::CLAP_SOUND:    return &shelf->hit_clap;
	case oshu::SLIDER_SOUND|oshu::NORMAL_SOUND:  return &shelf->slider_slide;
	case oshu::SLIDER_SOUND|oshu::WHISTLE_SOUND: return &shelf->slider_whistle;
	default:
		oshu_log_debug("unknown sample type %d", (int) type);
		return NULL;
	}
}

/**
 * Generate the file name for a sample given its characteristics.
 *
 * Sample outputs:
 * - `soft-hitclap9.wav`,
 * - `drum-hitwhistle.wav`.
 *
 * If the set or type is invalid, return an empty string.
 */
static std::string make_sample_file_name(enum oshu::sample_set_family set, int index, int type)
{
	/* Set name. */
	const char *set_name;
	switch (set) {
	case oshu::NORMAL_SAMPLE_SET: set_name = "normal"; break;
	case oshu::SOFT_SAMPLE_SET:   set_name = "soft"; break;
	case oshu::DRUM_SAMPLE_SET:   set_name = "drum"; break;
	default:
		oshu_log_debug("unknown sample set %d", (int) set);
		return {};
	}
	/* Type name. */
	const char *type_name;
	switch (type) {
	case oshu::HIT_SOUND|oshu::NORMAL_SOUND:  type_name = "hitnormal"; break;
	case oshu::HIT_SOUND|oshu::WHISTLE_SOUND: type_name = "hitwhistle"; break;
	case oshu::HIT_SOUND|oshu::FINISH_SOUND:  type_name = "hitfinish"; break;
	case oshu::HIT_SOUND|oshu::CLAP_SOUND:    type_name = "hitclap"; break;
	case oshu::SLIDER_SOUND|oshu::NORMAL_SOUND:  type_name = "sliderslide"; break;
	case oshu::SLIDER_SOUND|oshu::WHISTLE_SOUND: type_name = "sliderwhistle"; break;
	default:
		oshu_log_debug("unknown sample type %d", (int) set);
		return {};
	}
	/* Combine everything. */
	std::ostringstream os;
	os << set_name << '-' << type_name;
	if (index > 1)
		os << index;
	os << ".wav";
	return os.str();
}

/**
 * Look up a sample file from various directories.
 *
 * The special index 0 does not look into the beatmap directory but straight
 * into the default set. Any non-0 index will only look in the beatmap
 * directory.
 *
 * If no suitable sample file was found, an empty string is returned.
 */
static std::string locate_sample(oshu::sound_library *library, enum oshu::sample_set_family set, int index, int type)
{
	std::string filename = make_sample_file_name(set, index, type);
	if (filename.empty())
		return {};
	if (index > 0) {
		/* Check the current directory. */
		if (access(filename.c_str(), R_OK) == 0)
			return filename;
	} else {
		/* Check the installation's data directory. */
		std::string path = library->skin_directory + "/" + filename;
		if (access(path.c_str(), R_OK) == 0)
			return path;
	}
	return {};
}

int oshu::register_sample(struct oshu::sound_library *library, enum oshu::sample_set_family set, int index, int type)
{
	struct oshu::sound_room *room = get_room(library, set);
	if (!room)
		return -1;
	struct oshu::sound_shelf *shelf = find_shelf(room, index);
	if (!shelf)
		shelf = new_shelf(room, index);
	if (!shelf)
		return -1;
	struct oshu::sample **sample = get_sample(shelf, type);
	if (!sample)
		return -1;
	if (*sample) /* already loaded */
		return 0;
	std::string path = locate_sample(library, set, index, type);
	if (path.empty())
		return -1;
	oshu_log_debug("registering %s", path.c_str());
	*sample = (oshu::sample*) calloc(1, sizeof(**sample));
	assert (*sample != NULL);
	assert (library->format != NULL);
	int rc = oshu::load_sample(path.c_str(), library->format, *sample);
	if (rc < 0)
		oshu_log_debug("continuing the process with an empty sample");
	return 0;
}

void oshu::register_sound(struct oshu::sound_library *library, struct oshu::hit_sound *sound)
{
	int target = sound->additions & oshu::SOUND_TARGET;
	if (sound->additions & oshu::NORMAL_SOUND)
		oshu::register_sample(library, sound->sample_set, sound->index, target | oshu::NORMAL_SOUND);
	if (sound->additions & oshu::WHISTLE_SOUND)
		oshu::register_sample(library, sound->additions_set, sound->index, target | oshu::WHISTLE_SOUND);
	if (sound->additions & oshu::FINISH_SOUND)
		oshu::register_sample(library, sound->additions_set, sound->index, target | oshu::FINISH_SOUND);
	if (sound->additions & oshu::CLAP_SOUND)
		oshu::register_sample(library, sound->additions_set, sound->index, target | oshu::CLAP_SOUND);
}

static void populate_default(struct oshu::sound_library *library, enum oshu::sample_set_family set)
{
	oshu::register_sample(library, set, oshu::DEFAULT_SHELF, oshu::HIT_SOUND|oshu::NORMAL_SOUND);
	oshu::register_sample(library, set, oshu::DEFAULT_SHELF, oshu::HIT_SOUND|oshu::WHISTLE_SOUND);
	oshu::register_sample(library, set, oshu::DEFAULT_SHELF, oshu::HIT_SOUND|oshu::FINISH_SOUND);
	oshu::register_sample(library, set, oshu::DEFAULT_SHELF, oshu::HIT_SOUND|oshu::CLAP_SOUND);
	oshu::register_sample(library, set, oshu::DEFAULT_SHELF, oshu::SLIDER_SOUND|oshu::NORMAL_SOUND);
	oshu::register_sample(library, set, oshu::DEFAULT_SHELF, oshu::SLIDER_SOUND|oshu::WHISTLE_SOUND);
}

void oshu::populate_library(struct oshu::sound_library *library, struct oshu::beatmap *beatmap)
{
	int start = SDL_GetTicks();
	oshu_log_debug("loading the sample library");
	populate_default(library, oshu::NORMAL_SAMPLE_SET);
	populate_default(library, oshu::SOFT_SAMPLE_SET);
	populate_default(library, oshu::DRUM_SAMPLE_SET);
	for (struct oshu::hit *hit = beatmap->hits; hit; hit = hit->next) {
		if (hit->type & oshu::SLIDER_HIT) {
			for (int i = 0; i <= hit->slider.repeat; ++i)
				oshu::register_sound(library, &hit->slider.sounds[i]);
		}
		oshu::register_sound(library, &hit->sound);
	}
	int end = SDL_GetTicks();
	oshu_log_debug("done loading the library in %.3f seconds", (end - start) / 1000.);
}

/**
 * Find a sample given its attributes.
 *
 * If the wanted sample couldn't be found, return a sane replacement or, in the
 * worst case scenario, NULL.
 *
 * \sa oshu::play_sound
 */
struct oshu::sample* find_sample(struct oshu::sound_library *library, enum oshu::sample_set_family set, int index, int type)
{
	struct oshu::sample **sample {nullptr};
	struct oshu::sound_room *room = get_room(library, set);
	if (!room)
		return NULL;
	struct oshu::sound_shelf *shelf = find_shelf(room, index);
	if (!shelf)
		goto round2;
	sample = get_sample(shelf, type);
	if (sample && *sample)
		return *sample;
round2:
	shelf = find_shelf(room, oshu::DEFAULT_SHELF);
	if (!shelf)
		return NULL;
	sample = get_sample(shelf, type);
	if (!sample)
		return NULL;
	return *sample;
}

static void try_sound(struct oshu::sound_library *library, struct oshu::hit_sound *sound, struct oshu::audio *audio, enum oshu::sound_type flag)
{
	if (sound->additions & flag) {
		int target = sound->additions & oshu::SOUND_TARGET;
		oshu::sample_set_family set = (flag == oshu::NORMAL_SOUND) ? sound->sample_set : sound->additions_set;
		struct oshu::sample *sample = find_sample(library, set, sound->index, target | flag);
		if (!sample)
			return;
		else if (sound->additions & oshu::SLIDER_SOUND)
			oshu::play_loop(audio, sample, sound->volume);
		else
			oshu::play_sample(audio, sample, sound->volume);
	}
}

void oshu::play_sound(struct oshu::sound_library *library, struct oshu::hit_sound *sound, struct oshu::audio *audio)
{
	try_sound(library, sound, audio, oshu::NORMAL_SOUND);
	try_sound(library, sound, audio, oshu::WHISTLE_SOUND);
	try_sound(library, sound, audio, oshu::FINISH_SOUND);
	try_sound(library, sound, audio, oshu::CLAP_SOUND);
}

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
 * See #oshu_sound_library::skin_directory for details.
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
		os << OSHU_SKINS_DIRECTORY << "/" << env;
		std::string dir = os.str();
		if (!access(dir.c_str(), R_OK))
			return dir;
		else
			oshu_log_debug("could not find skin %s in directory %s", env, OSHU_SKINS_DIRECTORY);
	}
	/* The OSHU_SKIN variable wasn't helpful, so let's resort to the default skin. */
	std::ostringstream os;
	os << OSHU_SKINS_DIRECTORY << "/" << OSHU_DEFAULT_SKIN;
	return os.str();
}

void oshu_open_sound_library(struct oshu_sound_library *library, struct SDL_AudioSpec *format)
{
	library->skin_directory = skin_directory();
	oshu_log_debug("using skin directory %s", library->skin_directory.c_str());
	library->format = format;
}

static void free_sample(struct oshu_sample **sample)
{
	if (*sample) {
		oshu_destroy_sample(*sample);
		free(*sample);
		*sample = NULL;
	}
}

static void free_shelf(struct oshu_sound_shelf *shelf)
{
	free_sample(&shelf->hit_normal);
	free_sample(&shelf->hit_whistle);
	free_sample(&shelf->hit_finish);
	free_sample(&shelf->hit_clap);
	free_sample(&shelf->slider_slide);
	free_sample(&shelf->slider_whistle);
}

static void free_room(struct oshu_sound_room *room)
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

void oshu_close_sound_library(struct oshu_sound_library *library)
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
static struct oshu_sound_shelf* find_shelf(struct oshu_sound_room *room, int index)
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
static void grow_room(struct oshu_sound_room *room)
{
	assert (room->size <= room->capacity);
	if (room->size < room->capacity)
		return;
	room->capacity += 8;
	room->shelves = (oshu_sound_shelf*) realloc(room->shelves, room->capacity * sizeof(*room->shelves));
	assert (room->shelves != NULL);
	room->indices = (int*) realloc(room->indices, room->capacity * sizeof(*room->indices));
	assert (room->indices != NULL);
}

/**
 * Add a shelf in a room with a specific index.
 *
 * Return the pointer to the new shelf.
 */
static struct oshu_sound_shelf* new_shelf(struct oshu_sound_room *room, int index)
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
static struct oshu_sound_room* get_room(struct oshu_sound_library *library, enum oshu_sample_set_family set)
{
	switch (set) {
	case OSHU_NORMAL_SAMPLE_SET: return &library->normal;
	case OSHU_SOFT_SAMPLE_SET:   return &library->soft;
	case OSHU_DRUM_SAMPLE_SET:   return &library->drum;
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
static struct oshu_sample** get_sample(struct oshu_sound_shelf *shelf, int type)
{
	switch (type) {
	case OSHU_HIT_SOUND|OSHU_NORMAL_SOUND:  return &shelf->hit_normal;
	case OSHU_HIT_SOUND|OSHU_WHISTLE_SOUND: return &shelf->hit_whistle;
	case OSHU_HIT_SOUND|OSHU_FINISH_SOUND:  return &shelf->hit_finish;
	case OSHU_HIT_SOUND|OSHU_CLAP_SOUND:    return &shelf->hit_clap;
	case OSHU_SLIDER_SOUND|OSHU_NORMAL_SOUND:  return &shelf->slider_slide;
	case OSHU_SLIDER_SOUND|OSHU_WHISTLE_SOUND: return &shelf->slider_whistle;
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
static std::string make_sample_file_name(enum oshu_sample_set_family set, int index, int type)
{
	/* Set name. */
	const char *set_name;
	switch (set) {
	case OSHU_NORMAL_SAMPLE_SET: set_name = "normal"; break;
	case OSHU_SOFT_SAMPLE_SET:   set_name = "soft"; break;
	case OSHU_DRUM_SAMPLE_SET:   set_name = "drum"; break;
	default:
		oshu_log_debug("unknown sample set %d", (int) set);
		return {};
	}
	/* Type name. */
	const char *type_name;
	switch (type) {
	case OSHU_HIT_SOUND|OSHU_NORMAL_SOUND:  type_name = "hitnormal"; break;
	case OSHU_HIT_SOUND|OSHU_WHISTLE_SOUND: type_name = "hitwhistle"; break;
	case OSHU_HIT_SOUND|OSHU_FINISH_SOUND:  type_name = "hitfinish"; break;
	case OSHU_HIT_SOUND|OSHU_CLAP_SOUND:    type_name = "hitclap"; break;
	case OSHU_SLIDER_SOUND|OSHU_NORMAL_SOUND:  type_name = "sliderslide"; break;
	case OSHU_SLIDER_SOUND|OSHU_WHISTLE_SOUND: type_name = "sliderwhistle"; break;
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
static std::string locate_sample(oshu_sound_library *library, enum oshu_sample_set_family set, int index, int type)
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

int oshu_register_sample(struct oshu_sound_library *library, enum oshu_sample_set_family set, int index, int type)
{
	struct oshu_sound_room *room = get_room(library, set);
	if (!room)
		return -1;
	struct oshu_sound_shelf *shelf = find_shelf(room, index);
	if (!shelf)
		shelf = new_shelf(room, index);
	if (!shelf)
		return -1;
	struct oshu_sample **sample = get_sample(shelf, type);
	if (!sample)
		return -1;
	if (*sample) /* already loaded */
		return 0;
	std::string path = locate_sample(library, set, index, type);
	if (path.empty())
		return -1;
	oshu_log_debug("registering %s", path.c_str());
	*sample = (oshu_sample*) calloc(1, sizeof(**sample));
	assert (*sample != NULL);
	assert (library->format != NULL);
	int rc = oshu_load_sample(path.c_str(), library->format, *sample);
	if (rc < 0)
		oshu_log_debug("continuing the process with an empty sample");
	return 0;
}

void oshu_register_sound(struct oshu_sound_library *library, struct oshu_hit_sound *sound)
{
	int target = sound->additions & OSHU_SOUND_TARGET;
	if (sound->additions & OSHU_NORMAL_SOUND)
		oshu_register_sample(library, sound->sample_set, sound->index, target | OSHU_NORMAL_SOUND);
	if (sound->additions & OSHU_WHISTLE_SOUND)
		oshu_register_sample(library, sound->additions_set, sound->index, target | OSHU_WHISTLE_SOUND);
	if (sound->additions & OSHU_FINISH_SOUND)
		oshu_register_sample(library, sound->additions_set, sound->index, target | OSHU_FINISH_SOUND);
	if (sound->additions & OSHU_CLAP_SOUND)
		oshu_register_sample(library, sound->additions_set, sound->index, target | OSHU_CLAP_SOUND);
}

static void populate_default(struct oshu_sound_library *library, enum oshu_sample_set_family set)
{
	oshu_register_sample(library, set, OSHU_DEFAULT_SHELF, OSHU_HIT_SOUND|OSHU_NORMAL_SOUND);
	oshu_register_sample(library, set, OSHU_DEFAULT_SHELF, OSHU_HIT_SOUND|OSHU_WHISTLE_SOUND);
	oshu_register_sample(library, set, OSHU_DEFAULT_SHELF, OSHU_HIT_SOUND|OSHU_FINISH_SOUND);
	oshu_register_sample(library, set, OSHU_DEFAULT_SHELF, OSHU_HIT_SOUND|OSHU_CLAP_SOUND);
	oshu_register_sample(library, set, OSHU_DEFAULT_SHELF, OSHU_SLIDER_SOUND|OSHU_NORMAL_SOUND);
	oshu_register_sample(library, set, OSHU_DEFAULT_SHELF, OSHU_SLIDER_SOUND|OSHU_WHISTLE_SOUND);
}

void oshu_populate_library(struct oshu_sound_library *library, struct oshu_beatmap *beatmap)
{
	int start = SDL_GetTicks();
	oshu_log_debug("loading the sample library");
	populate_default(library, OSHU_NORMAL_SAMPLE_SET);
	populate_default(library, OSHU_SOFT_SAMPLE_SET);
	populate_default(library, OSHU_DRUM_SAMPLE_SET);
	for (struct oshu_hit *hit = beatmap->hits; hit; hit = hit->next) {
		if (hit->type & OSHU_SLIDER_HIT) {
			for (int i = 0; i <= hit->slider.repeat; ++i)
				oshu_register_sound(library, &hit->slider.sounds[i]);
		}
		oshu_register_sound(library, &hit->sound);
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
 * \sa oshu_play_sound
 */
struct oshu_sample* find_sample(struct oshu_sound_library *library, enum oshu_sample_set_family set, int index, int type)
{
	struct oshu_sample **sample {nullptr};
	struct oshu_sound_room *room = get_room(library, set);
	if (!room)
		return NULL;
	struct oshu_sound_shelf *shelf = find_shelf(room, index);
	if (!shelf)
		goto round2;
	sample = get_sample(shelf, type);
	if (sample && *sample)
		return *sample;
round2:
	shelf = find_shelf(room, OSHU_DEFAULT_SHELF);
	if (!shelf)
		return NULL;
	sample = get_sample(shelf, type);
	if (!sample)
		return NULL;
	return *sample;
}

static void try_sound(struct oshu_sound_library *library, struct oshu_hit_sound *sound, struct oshu_audio *audio, enum oshu_sound_type flag)
{
	if (sound->additions & flag) {
		int target = sound->additions & OSHU_SOUND_TARGET;
		oshu_sample_set_family set = (flag == OSHU_NORMAL_SOUND) ? sound->sample_set : sound->additions_set;
		struct oshu_sample *sample = find_sample(library, set, sound->index, target | flag);
		if (!sample)
			return;
		else if (sound->additions & OSHU_SLIDER_SOUND)
			oshu_play_loop(audio, sample, sound->volume);
		else
			oshu_play_sample(audio, sample, sound->volume);
	}
}

void oshu_play_sound(struct oshu_sound_library *library, struct oshu_hit_sound *sound, struct oshu_audio *audio)
{
	try_sound(library, sound, audio, OSHU_NORMAL_SOUND);
	try_sound(library, sound, audio, OSHU_WHISTLE_SOUND);
	try_sound(library, sound, audio, OSHU_FINISH_SOUND);
	try_sound(library, sound, audio, OSHU_CLAP_SOUND);
}

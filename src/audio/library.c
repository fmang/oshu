/**
 * \file audio/library.c
 * \ingroup audio_library
 */

#include "../config.h"

#include "audio/library.h"
#include "audio/audio.h"
#include "audio/sample.h"
#include "log.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

void oshu_open_sound_library(struct oshu_sound_library *library, struct SDL_AudioSpec *format)
{
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
	room->shelves = realloc(room->shelves, room->capacity * sizeof(*room->shelves));
	assert (room->shelves != NULL);
	room->indices = realloc(room->indices, room->capacity * sizeof(*room->indices));
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
 * *size* is the size of the character *buffer*, and it should be at least 32
 * bytes. If the buffer is too short, an error is printed and -1 is returned.
 *
 * Sample outputs:
 * - `soft-hitclap9.wav`,
 * - `drum-hitwhistle.wav`.
 */
static int make_sample_file_name(enum oshu_sample_set_family set, int index, int type, char *buffer, int size)
{
	/* Set name. */
	const char *set_name;
	switch (set) {
	case OSHU_NORMAL_SAMPLE_SET: set_name = "normal"; break;
	case OSHU_SOFT_SAMPLE_SET:   set_name = "soft"; break;
	case OSHU_DRUM_SAMPLE_SET:   set_name = "drum"; break;
	default:
		oshu_log_debug("unknown sample set %d", (int) set);
		return -1;
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
		return -1;
	}
	/* Combine everything. */
	int rc;
	if (index == 1) /* augh */
		rc = snprintf(buffer, size, "%s-%s.wav", set_name, type_name);
	else
		rc = snprintf(buffer, size, "%s-%s%d.wav", set_name, type_name, index);
	if (rc < 0) {
		oshu_log_error("sample file name formatting failed");
		return -1;
	} else if (rc >= size) {
		oshu_log_error("sample file name was truncated");
		return -1;
	}
	return 0;
}

/**
 * Look up a sample file from various directories.
 *
 * Return a dynamically allocated buffer with the path to a WAV file on
 * success, NULL on failure.
 */
static char* locate_sample(enum oshu_sample_set_family set, int index, int type)
{
	char filename[32];
	if (make_sample_file_name(set, index, type, filename, sizeof(filename)) < 0)
		return NULL;
	/* 1. Check the current directory. */
	if (access(filename, R_OK) == 0)
		return strdup(filename);
	/* 2. Check the installation's data directory. */
	char *path;
	int rc = asprintf(&path, "%s/samples/%s", PKGDATADIR, filename);
	assert (rc >= 0);
	if (access(path, R_OK) == 0)
		return path;
	free(path);
	/* 3. Fail. */
	return NULL;
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
	char *path = locate_sample(set, index, type);
	if (!path)
		return -1;
	oshu_log_debug("registering %s", path);
	*sample = calloc(1, sizeof(**sample));
	assert (*sample != NULL);
	assert (library->format != NULL);
	int rc = oshu_load_sample(path, library->format, *sample);
	free(path);
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
	populate_default(library, OSHU_NORMAL_SAMPLE_SET);
	populate_default(library, OSHU_SOFT_SAMPLE_SET);
	for (struct oshu_hit *hit = beatmap->hits; hit; hit = hit->next) {
		if (hit->type & OSHU_SLIDER_HIT) {
			for (int i = 0; i <= hit->slider.repeat; ++i)
				oshu_register_sound(library, &hit->slider.sounds[i]);
		}
		oshu_register_sound(library, &hit->sound);
	}
}

/**
 * Find a sample given its attributes.
 *
 * If the wanted sample couldn't be found, return a sane replacement or, in the
 * worst case scenario, NULL.
 *
 * \sa oshu_play_sound
 */
struct oshu_sample* find_sample(struct oshu_sound_library *library, enum oshu_sample_set_family set, int index, enum oshu_sound_type type)
{
	struct oshu_sound_room *room = get_room(library, set);
	if (!room)
		return NULL;
	struct oshu_sound_shelf *shelf = find_shelf(room, index);
	if (!shelf)
		goto round2;
	struct oshu_sample **sample = get_sample(shelf, type);
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

void oshu_play_sound(struct oshu_sound_library *library, struct oshu_hit_sound *sound, struct oshu_audio *audio)
{
	int target = sound->additions & OSHU_SOUND_TARGET;
	struct oshu_sample *sample;
	if (sound->additions & OSHU_NORMAL_SOUND)
		sample = find_sample(library, sound->sample_set, sound->index, target | OSHU_NORMAL_SOUND);
	if (sound->additions & OSHU_WHISTLE_SOUND)
		sample = find_sample(library, sound->additions_set, sound->index, target | OSHU_WHISTLE_SOUND);
	if (sound->additions & OSHU_FINISH_SOUND)
		sample = find_sample(library, sound->additions_set, sound->index, target | OSHU_FINISH_SOUND);
	if (sound->additions & OSHU_CLAP_SOUND)
		sample = find_sample(library, sound->additions_set, sound->index, target | OSHU_CLAP_SOUND);
	if (sample) {
		if (sound->additions & OSHU_SLIDER_SOUND)
			oshu_play_loop(audio, sample, sound->volume);
		else
			oshu_play_sample(audio, sample, sound->volume);
	}
}

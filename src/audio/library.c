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

static void free_shelf(struct oshu_sound_shelf *shelf)
{
	if (shelf->normal)
		oshu_destroy_sample(shelf->normal);
	if (shelf->whistle)
		oshu_destroy_sample(shelf->whistle);
	if (shelf->finish)
		oshu_destroy_sample(shelf->finish);
	if (shelf->clap)
		oshu_destroy_sample(shelf->clap);
}

static void free_room(struct oshu_sound_room *room)
{
	if (!room->shelves)
		return;
	for (int i = 0; i < room->size; ++i)
		free_shelf(&room->shelves[i]);
	free(room->shelves);
	free(room->indices);
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
		oshu_log_warning("unknown sample set %d", (int) set);
		return NULL;
	}
}

/**
 * Locate the sample on a shelf given its type.
 *
 * If the sample wasnn't found because its type doesn't exist, return NULL.
 */
static struct oshu_sample** get_sample(struct oshu_sound_shelf *shelf, enum oshu_sample_type type)
{
	switch (type) {
	case OSHU_NORMAL_SAMPLE:  return &shelf->normal;
	case OSHU_WHISTLE_SAMPLE: return &shelf->whistle;
	case OSHU_FINISH_SAMPLE:  return &shelf->finish;
	case OSHU_CLAP_SAMPLE:    return &shelf->clap;
	default:
		oshu_log_warning("unknown sample type %d", (int) type);
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
static int make_sample_file_name(enum oshu_sample_set_family set, int index, enum oshu_sample_type type, char *buffer, int size)
{
	/* Set name. */
	const char *set_name;
	switch (set) {
	case OSHU_NORMAL_SAMPLE_SET: set_name = "normal"; break;
	case OSHU_SOFT_SAMPLE_SET:   set_name = "soft"; break;
	case OSHU_DRUM_SAMPLE_SET:   set_name = "drum"; break;
	default:
		oshu_log_warning("unknown sample set %d", (int) set);
		return -1;
	}
	/* Type name. */
	const char *type_name;
	switch (type) {
	case OSHU_NORMAL_SAMPLE:  type_name = "normal"; break;
	case OSHU_WHISTLE_SAMPLE: type_name = "whistle"; break;
	case OSHU_FINISH_SAMPLE:  type_name = "finish"; break;
	case OSHU_CLAP_SAMPLE:    type_name = "clap"; break;
	default:
		oshu_log_warning("unknown sample type %d", (int) set);
		return -1;
	}
	/* Combine everything. */
	int rc;
	if (index == 1) /* augh */
		rc = snprintf(buffer, size, "%s-hit%s.wav", set_name, type_name);
	else
		rc = snprintf(buffer, size, "%s-hit%s%d.wav", set_name, type_name, index);
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
static char* locate_sample(enum oshu_sample_set_family set, int index, enum oshu_sample_type type)
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

int oshu_register_sample(struct oshu_sound_library *library, enum oshu_sample_set_family set, int index, enum oshu_sample_type type)
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
	if (rc < 0) {
		free(*sample);
		*sample = NULL;
		return -1;
	}
	return 0;
}

void oshu_register_sound(struct oshu_sound_library *library, struct oshu_hit_sound *sound)
{
	if (sound->additions & OSHU_NORMAL_SAMPLE)
		oshu_register_sample(library, sound->sample_set, sound->index, OSHU_NORMAL_SAMPLE);
	if (sound->additions & OSHU_WHISTLE_SAMPLE)
		oshu_register_sample(library, sound->additions_set, sound->index, OSHU_WHISTLE_SAMPLE);
	if (sound->additions & OSHU_FINISH_SAMPLE)
		oshu_register_sample(library, sound->additions_set, sound->index, OSHU_FINISH_SAMPLE);
	if (sound->additions & OSHU_CLAP_SAMPLE)
		oshu_register_sample(library, sound->additions_set, sound->index, OSHU_CLAP_SAMPLE);
}

static void populate_default(struct oshu_sound_library *library, enum oshu_sample_set_family set)
{
	oshu_register_sample(library, set, 1, OSHU_NORMAL_SAMPLE);
	oshu_register_sample(library, set, 1, OSHU_WHISTLE_SAMPLE);
	oshu_register_sample(library, set, 1, OSHU_FINISH_SAMPLE);
	oshu_register_sample(library, set, 1, OSHU_CLAP_SAMPLE);
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
struct oshu_sample* find_sample(struct oshu_sound_library *library, enum oshu_sample_set_family set, int index, enum oshu_sample_type type)
{
	struct oshu_sound_room *room = get_room(library, set);
	if (!room)
		return NULL;
	struct oshu_sound_shelf *shelf = find_shelf(room, index);
	if (!shelf)
		return NULL;
	struct oshu_sample **sample = get_sample(shelf, type);
	if (!sample)
		return NULL;
	return *sample;
}

/**
 * Find a single sample and play it.
 */
static void play_sample(struct oshu_sound_library *library, enum oshu_sample_set_family set, int index, enum oshu_sample_type type, float volume, struct oshu_audio *audio)
{
	struct oshu_sample *sample = find_sample(library, set, index, type);
	if (sample)
		oshu_play_sample(audio, sample, volume);
}

void oshu_play_sound(struct oshu_sound_library *library, struct oshu_hit_sound *sound, struct oshu_audio *audio)
{
	if (sound->additions & OSHU_NORMAL_SAMPLE)
		play_sample(library, sound->sample_set, sound->index, OSHU_NORMAL_SAMPLE, sound->volume, audio);
	if (sound->additions & OSHU_WHISTLE_SAMPLE)
		play_sample(library, sound->additions_set, sound->index, OSHU_WHISTLE_SAMPLE, sound->volume, audio);
	if (sound->additions & OSHU_FINISH_SAMPLE)
		play_sample(library, sound->additions_set, sound->index, OSHU_FINISH_SAMPLE, sound->volume, audio);
	if (sound->additions & OSHU_CLAP_SAMPLE)
		play_sample(library, sound->additions_set, sound->index, OSHU_CLAP_SAMPLE, sound->volume, audio);
}

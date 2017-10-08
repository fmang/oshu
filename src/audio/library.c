/**
 * \file audio/library.c
 * \ingroup audio_library
 */

#include "audio/library.h"
#include "log.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void oshu_open_sound_library(struct oshu_sound_library *library, struct SDL_AudioSpec *format)
{
	library->format = format;
}

static void free_shelf(struct oshu_sound_shelf *shelf)
{
	free(shelf->normal);
	free(shelf->whistle);
	free(shelf->finish);
	free(shelf->clap);
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
 * Generate the file name for a sample given its characteristics.
 *
 * *size* is the size of the character *buffer*, and it should be at least 32
 * bytes. If the buffer is too short, an error is printed and -1 is returned.
 *
 * Sample outputs:
 * - `soft-hitclap9.wav`,
 * - `drum-hitwhistle.wav`.
 */
static int sample_file_name(enum oshu_sample_set_family set, int index, enum oshu_sample_type type, char *buffer, int size)
{
	/* Set name. */
	const char *set_name;
	switch (set) {
	case OSHU_NORMAL_SAMPLE_SET: set_name = "normal"; break;
	case OSHU_SOFT_SAMPLE_SET:   set_name = "soft"; break;
	case OSHU_DRUM_SAMPLE_SET:   set_name = "drum"; break;
	default:
		oshu_log_error("unknown sample set %d", (int) set);
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
		oshu_log_error("unknown sample type %d", (int) set);
		return -1;
	}
	/* Combine everything. */
	int rc;
	if (!index) /* augh */
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

int oshu_register_sample(struct oshu_sound_library *library, enum oshu_sample_set_family set, int index, enum oshu_sample_type type)
{
	return 0;
}

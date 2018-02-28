/**
 * \file audio/library.h
 * \ingroup audio_library
 */

#pragma once

#include "beatmap/beatmap.h"

#include <string>

struct oshu_audio;
struct oshu_sample;

struct SDL_AudioSpec;

/**
 * \defgroup audio_library Library
 * \ingroup audio
 *
 * \brief
 * Manage a collection of sound samples.
 *
 * The library is organized into rooms, one for each sample set family (normal,
 * soft, drum). Each room contains shelves, even though some rooms are
 * sometimes empty. Shelves are identified by their index number. Each shelf
 * contains up to 4 samples.
 *
 * Here's an idea of the library's map:
 *
 * \dot
 * digraph {
 * 	rankdir=TB
 * 	node [shape=rect]
 * 	Hall -> "Normal room"
 * 	Hall -> "Soft room"
 * 	Hall -> "Drum room"
 * 	"Normal room" -> "Shelf #1"
 * 	"Shelf #1" -> "Normal sound"
 * 	"Shelf #1" -> "Whistle sound"
 * 	"Shelf #1" -> "Finish sound"
 * 	"Shelf #1" -> "Clap sound"
 * }
 * \enddot
 *
 * \{
 */

enum oshu_sound_shelf_index {
	/**
	 * Fallback shelf when a sample is missing from a custom shelf.
	 *
	 * The default shelf is built from the default resources, or ideally
	 * the skin. It is different from shelf 1 which uses the sample files
	 * in the beatmap directory.
	 */
	OSHU_DEFAULT_SHELF = 0,
};

/**
 * Each sample in a shelf is a pointer to a sample.
 *
 * These pointers are NULL when no sample is available.
 */
struct oshu_sound_shelf {
	struct oshu_sample *hit_normal;
	struct oshu_sample *hit_whistle;
	struct oshu_sample *hit_finish;
	struct oshu_sample *hit_clap;
	struct oshu_sample *slider_slide;
	struct oshu_sample *slider_whistle;
};

/**
 * A room is a collection of shelves. Each shelf is indexed by a number. No two
 * shelves may share the same index. The index space is not necessarily
 * contiguous. You may see in practice the default 0 shelf, and some 99 shelf
 * popping out of nowhere.
 *
 * More concretely, a room is a dynamically-sized array of shelves.
 */
struct oshu_sound_room {
	/**
	 * Location of the shelves.
	 *
	 * The elements are sorted in an arbitrary order, so you must use
	 * #indices to find the shelf you're looking for.
	 *
	 * \sa size
	 */
	struct oshu_sound_shelf *shelves;
	/**
	 * Indices of the shelves.
	 *
	 * `indices[i]` is the index of `shelves[i]`.
	 *
	 * The same index should not appear more than once, and not every index
	 * may appear.
	 *
	 * Since the structure is not indexed beyond that, you have to browse
	 * the whole array every time you're looking for a specific shelf.
	 *
	 * \sa size
	 */
	int *indices;
	/**
	 * Number of shelves.
	 *
	 * This is also the minimum size of the #shelves and #indices arrays.
	 */
	int size;
	/**
	 * Actual size of the memory block of #shelves and #indices.
	 *
	 * Rooms may grow using a spatio-temporal expansion often called
	 * *realloc*.
	 */
	int capacity;
};

/**
 * The sound sample library.
 *
 * The library is composed of 3 rooms, one per sample set family.
 *
 * \sa oshu_sample_set_family
 */
struct oshu_sound_library {
	/**
	 * Path to the skin directory, containing the samples.
	 *
	 * It may, but should not, end with a trailing slash.
	 */
	std::string skin_directory;
	/**
	 * Format of the samples in the library.
	 */
	struct SDL_AudioSpec *format;
	struct oshu_sound_room normal;
	struct oshu_sound_room soft;
	struct oshu_sound_room drum;
};

/**
 * Initialize a sound library.
 *
 * The format define the format of the samples that are going to be stored. The
 * point is stored without duplication of the underlying data. Make sure it
 * stays alive!
 *
 * The library object must be zero-initalized.
 *
 * \sa oshu_close_sound_library
 */
void oshu_open_sound_library(struct oshu_sound_library *library, struct SDL_AudioSpec *format);

/**
 * Delete all the loaded samples from the library.
 */
void oshu_close_sound_library(struct oshu_sound_library *library);

/**
 * Locate a sample on the filesystem and insert it into the library.
 *
 * *type* is an OR'd combination of #oshu_sound_type.
 *
 * \sa oshu_register_sounds
 */
int oshu_register_sample(struct oshu_sound_library *library, enum oshu_sample_set_family set, int index, int type);

/**
 * Load every sample required to play a hit sound effect.
 *
 * \sa oshu_register_sample
 * \sa oshu_populate_library
 */
void oshu_register_sound(struct oshu_sound_library *library, struct oshu_hit_sound *sound);

/**
 * Find every sample reference into a beatmap and load them into the library.
 *
 * \sa oshu_register_sounds
 */
void oshu_populate_library(struct oshu_sound_library *library, struct oshu_beatmap *beatmap);

/**
 * Play all the samples associated to the hit sound.
 *
 * If one of the required samples wasn't found, it is ignored.
 *
 * If the sound is for a slider, its sample is looped until you call #oshu_stop_loop.
 *
 * \sa oshu_find_sample
 */
void oshu_play_sound(struct oshu_sound_library *library, struct oshu_hit_sound *sound, struct oshu_audio *audio);

/** \} */

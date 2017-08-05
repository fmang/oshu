/**
 * \file beatmap.h
 * \ingroup beatmap
 */

#pragma once

/** \defgroup beatmap Beatmap
 *
 * \brief
 * Define and load \e .osu beatmap files.
 *
 * Check there for the format reference:
 * https://osu.ppy.sh/help/wiki/osu!_File_Formats
 *
 * In particular this one:
 * https://osu.ppy.sh/help/wiki/osu!_File_Formats/Osu_(file_format)
 *
 * To find sample files, just download any beatmap you find on the official
 * Osu! website. The .osz files are just ZIP files. After you extract them,
 * you'll find the .osu files we're going to parse in this module.
 *
 * A \e .osu file is some kind of pseudo-INI, with sections written like
 * `[Metadata]` and key values, except they're written `key: value`. Some
 * sections are completely unlike INI, but pretty something like CSV.
 *
 * Because the format is so chaotic, the parser will happily ignore things it
 * doesn't understand, and generate a few warnings.
 *
 * \{
 */

enum oshu_mode {
	OSHU_MODE_OSU = 0,
	OSHU_MODE_TAIKO = 1,
	OSHU_MODE_CATCH_THE_BEAT = 2,
	OSHU_MODE_MANIA = 3,
};

/**
 * One timing point.
 *
 * This structure is a complete transcription of the timing point description
 * in the documentation, even though it's not used yet.
 */
struct oshu_timing_point {
	int offset; /**< Milliseconds. */
	float ms_per_beat;
	int meter;
	int sample_type;
	int sample_set;
	int volume; /**< 0-100. */
	int inherited;
	int kiai;
};

/**
 * Flags defining the type of a hit object.
 *
 * To check if it's a circle, you should use `type & OSHU_HIT_CIRCLE` rather
 * than check for equality, because it will often be combined with \ref
 * OSHU_HIT_NEW_COMBO.
 */
enum oshu_hit_type {
	OSHU_HIT_CIRCLE = 0b1,
	OSHU_HIT_SLIDER = 0b10,
	OSHU_HIT_NEW_COMBO = 0b100,
	OSHU_HIT_SPINNER = 0b1000,
	OSHU_HIT_COMBO_MASK = 0b1110000, /**< How many combos to skip. */
	OSHU_HIT_HOLD = 0b10000000, /**< Mania mode only. */
};

enum oshu_hit_state {
	OSHU_HIT_INITIAL = 0,
	OSHU_HIT_GOOD,
	OSHU_HIT_MISSED,
};

/**
 * One hit object, and one link in the beatmap.
 *
 * This structure actually defines a link in the linked list of all hit
 * objects, sorted by chronological order.
 *
 * It is currently very limited, because it doesn't support sliders, or the
 * sample set.
 */
struct oshu_hit {
	int x; /**< 0-512 pixels, inclusive. */
	int y; /**< 0-384 pixels, inclusive. */
	int time; /**< Milliseconds. */
	int type; /**< Combination of flags from \ref oshu_hit_type. */
	enum oshu_hit_state state;
	struct oshu_hit *next;
};

/**
 * One beatmap, from its metadata to hit objects.
 *
 * This structure is not limited to the raw parsed data, but also provides
 * space for the game state, like which objects was hit and when.
 *
 * It also focuses on ease of use by SDL when rendering rather than providing
 * an accurate abstract syntax tree of the original file.
 *
 * Most string values are dynamically allocated inside this structure. Make
 * sure you free it with \ref oshu_beatmap_free.
 */
struct oshu_beatmap {
	int version;
	struct {
		char *audio_filename;
		enum oshu_mode mode;
	} general;
	struct {
		/**
		 * Makes the link between a slider's pixel length and the time.
		 * 1 beat maps to 100 pixels multiplied by this factor.
		 *
		 * Therefore, the total duration in beats of a slider is
		 * `pixel_length / (100 * slider_multiplier)`. Multiply this by
		 * the milliseconds per beat of of the beatmap and you'll get
		 * that duration is milliseconds.
		 *
		 * According to the official documentation, its default value
		 * is 1.4.
		 */
		float slider_multiplier;
	} difficulty;
	struct oshu_hit *hits;
	struct oshu_hit *hit_cursor;
};

/**
 * Take a path to a \e .osu file, open it and parse it.
 *
 * The parsed beatmap is returned by setting `*beatmap` to point to a newly
 * allocated beatmap object.
 *
 * On failure, the beatmap is automatically free'd.
 */
int oshu_beatmap_load(const char *path, struct oshu_beatmap **beatmap);

/**
 * Free the beatmap object and every other dynamically allocated object it
 * refers to.
 *
 * Sets `*beatmap` to *NULL.
 */
void oshu_beatmap_free(struct oshu_beatmap **beatmap);

/** \} */

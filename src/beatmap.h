#pragma once

/** @defgroup beatmap Beatmap
 *
 * Check there for the format reference:
 * https://osu.ppy.sh/help/wiki/osu!_File_Formats
 *
 * To find sample files, just download any beatmap you find on the official
 * Osu! website. The .osz files are just ZIP files. After you extract them,
 * you'll find the .osu files we're going to parse in this module.
 *
 * @{
 */

enum oshu_mode {
	OSHU_MODE_OSU = 0,
};

struct oshu_timing_point {
	int offset; /* ms */
	float ms_per_beat;
	int meter;
	int sample_type;
	int sample_set;
	int volume; /* 0-100 */
	int inherited;
	int kiai;
};

enum oshu_hit_type {
	OSHU_HIT_CIRCLE = 0b1,
	OSHU_HIT_SLIDER = 0b10,
	OSHU_HIT_NEW_COMBO = 0b100,
	OSHU_HIT_SPINNER = 0b1000,
	OSHU_HIT_COMBO_MASK = 0b1110000,
	OSHU_HIT_HOLD = 0b10000000,
};

struct oshu_hit {
	int x; /* 0-512 */
	int y; /* 0-384 */
	int time; /* ms */
	int type;
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
};

int oshu_beatmap_load(const char *path, struct oshu_beatmap **beatmap);

void oshu_beatmap_free(struct oshu_beatmap **beatmap);

/** @} */

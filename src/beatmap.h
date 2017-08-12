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

/**
 * The supported modes by the osu! beatmap file format.
 *
 * The value of the constants match the way they're written as integers in the
 * beatmap.
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
 * than check for equality, because it will often be combined with
 * #OSHU_HIT_NEW_COMBO.
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
	int type; /**< Combination of flags from #oshu_hit_type. */
	enum oshu_hit_state state;
	struct oshu_hit *next;
};

/**
 * \brief Complete definition of the [Metadata] section.
 *
 * Every character string inside this section is either NULL or a reference to
 * a dynamically allocated string.
 *
 * This structure should *own* the strings it references, so make sure you
 * `strdup` your strings before filling it.
 *
 * All the strings are encoded in UTF-8.
 */
struct oshu_metadata {
	/**
	 * \brief ASCII representation of the title.
	 *
	 * This is what the user will usually see unless they can read
	 * Japanese.
	 */
	char *title;
	/**
	 * The true #title, with kanji and everything. It looks nicer but may
	 * be unreadable to many.
	 */
	char *title_unicode;
	/**
	 * \brief ASCII name of the artist of the song.
	 */
	char *artist;
	/**
	 * The true #artist name.
	 */
	char *artist_unicode;
	/**
	 * Username of the creator of the beatmap, also known as the *mapper*.
	 */
	char *creator;
	/**
	 * \brief Name of the beatmap's difficulty.
	 *
	 * It's often something like *Easy* or *Normal*, but is actually free
	 * form and may contain fancier values like *Sunrise*, whatever that
	 * means.
	 */
	char *version;
	/**
	 * \briefOrigin of the song.
	 *
	 * Might be the name of a series like *Touhou* which has got an
	 * impressive amount of fan songs.
	 */
	char *source;
	/**
	 * Array of all the tags, for easy searching of beatmaps.
	 *
	 * In the beatmap file, tags are separated by spaces.
	 *
	 * May be *NULL*.
	 */
	char **tags;
	/**
	 * Beatmap ID of the beatmap, from osu!'s official beatmap repository.
	 */
	int beatmap_id;
	/**
	 * ID of the beatmap set. This is the prefix of the *.osz* files you
	 * find on osu!'s website web when downloaded beatmaps or beatmap
	 * packs.
	 */
	int beatmap_set_id;
};

/**
 * \brief Complete definition of the [Difficulty] section.
 *
 * A lot of its fields use an obscure unit. Because oshu! is still a prototype,
 * we'll try to guess at first, then maybe compare later, or ask around.
 */
struct oshu_difficulty {
	/**
	 * Health point drain rate.
	 *
	 * This is how much a missed mark lowers your health bar until you
	 * lose, especially in the standard osu! game mode.
	 *
	 * It's a float, but its unit is obscure.
	 *
	 * Defaults to 1, whatever that means.
	 */
	double hp_drain_rate;
	/**
	 * \brief Radius of the hit object's circles, in pixels.
	 *
	 * In the beatmap file, it's a float field `CircleSize` with a strange
	 * unit. For now, let's assume it's the radius divided by 10. It's the
	 * parser's job to convert it.
	 *
	 * Defaults to 32.
	 */
	double circle_radius;
	/**
	 * \brief Time tolerance to make a click good, in seconds.
	 *
	 * > OverallDifficulty (Float) specifies the amount of time allowed to click a hit object on time.
	 *
	 * In the beatmap file, it's an obscure float, so let's say an *overall
	 * difficulty* of 5 is 0.1 seconds, and get proportional from here.
	 *
	 * Defaults to 0.1 seconds.
	 */
	double leniency;
	/**
	 * \brief Duration of the approach circle, in seconds.
	 *
	 * > ApproachRate (Float) specifies the amount of time taken for the
	 * > approach circle and hit object to appear.
	 *
	 * Another obscure field. Let's say an approach rate of 5 maps to 1
	 * second. So, this field will receive on fifth of the value in the
	 * beatmap.
	 *
	 * Defaults to 1 second.
	 */
	double approach_rate;
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
	 *
	 * It should not be used outside of the beatmap module. The parser
	 * should use this value to compute the length of the sliders when
	 * creating them.
	 */
	double slider_multiplier;
	/**
	 * > SliderTickRate (Float) specifies how often slider ticks appear.
	 * > Default value is 1.
	 *
	 * I guess its unit is *ticks per beat*.
	 */
	double slider_tick_rate;
};

struct oshu_event {
};

struct oshu_timing_point {
};

struct oshu_color {
};

enum oshu_sampleset_category {
	OSHU_SAMPLESET_NORMAL,
	OSHU_SAMPLESET_SOFT,
	OSHU_SAMPLESET_DRUM,
};

/**
 * \brief One beatmap, from its metadata to hit objects.
 *
 * This structure is not limited to the raw parsed data, but also provides
 * space for the game state, like which objects was hit and when.
 *
 * It also focuses on ease of use by SDL when rendering rather than providing
 * an accurate abstract syntax tree of the original file.
 *
 * Most string values are dynamically allocated inside this structure. Make
 * sure you free it with #oshu_beatmap_free.
 */
struct oshu_beatmap {
	/**
	 * The version written in the header of every osu beatmap file.
	 * Today it's something around 14.
	 */
	int version;
	/**
	 * \brief Name of the audio file relative to the beatmap's directory.
	 *
	 * It should not contain any slash.
	 *
	 * Because of the structure of the INI file, it's not clear whether
	 * spaces should be trimmed or not. Someone could make a file named `
	 * .mp3` for all I know. Still, INI is unclear about it.
	 *
	 * One more thing, because such annoying beatmaps do exist: with
	 * case-insensitive filesystems like NTFS, a mis-cased filename would
	 * work on Windows and be considered a working beatmap even though on
	 * ext4 it wouldn't work. The parser cannot detect that but some layer
	 * between the beatmap loader and the audio loader should maybe take
	 * that into account.
	 *
	 * It is obviously mandatory, and a file missing that field should
	 * trigger a parsing error.
	 */
	char *audio_filename;
	/**
	 * \brief Length of the silence before the song starts playing, in
	 * seconds.
	 *
	 * Some beatmaps start with a hit object at the first note in the
	 * song, which was rough in oshu! 1.0.0 since a tenth of second after
	 * the window appeared, you'd see a miss.
	 *
	 * In the beatmap file, it's an integral number of milliseconds. 0 is a
	 * sane default.
	 */
	double audio_lead_in;
	/**
	 * When to start playing the song in the song selection menu, if there
	 * were one. In seconds, even though it's an integral number of
	 * milliseconds in the beatmap file. Defaults to 0.
	 */
	double preview_time;
	/**
	 * Specifies whether or not a countdown occurs before the first hit
	 * object appears. Linked to #audio_lead_in. Defaults to 0.
	 *
	 * In both the beatmap file and in oshu!, 0 means false and 1 means
	 * true.
	 */
	int countdown;
	/**
	 * Default sample set used in the beatmap. Should be written as
	 * `Normal`, `Soft`, `Drum` in the beatmap.
	 *
	 * Note that timing points or specific hit objects may use their own
	 * sampleset, so you must check the hit object first before resorting
	 * to this variable.
	 *
	 * Let's default to #OSHU_SAMPLESET_SOFT.
	 */
	enum oshu_sampleset_category sampleset;
	/**
	 * From the official documentation:
	 *
	 * > StackLeniency (Float) is how often closely placed hit objects will
	 * > be stacked together.
	 *
	 * Not sure what it means.
	 */
	double stack_leniency;
	/**
	 * The game mode. Today, only the standard osu! game is supported.
	 *
	 * It is written as a number between 0 and 3, and matches the values in
	 * #oshu_mode.
	 */
	enum oshu_mode mode;
	/**
	 * Pseudo-boolean. Unused in oshu!.
	 *
	 * > LetterboxInBreaks (Boolean) specifies whether the letterbox
	 * > appears during breaks.
	 */
	int letterbox_in_breaks;
	/**
	 * Pseudo-boolean. Unused in oshu!.
	 *
	 * > WidescreenStoryboard (Boolean) specifies whether or not the
	 * > storyboard should be widescreen.
	 */
	int widescreen_storyboard;
	/**
	 * \brief [Metadata] section.
	 */
	struct oshu_metadata metadata;
	/**
	 * \brief [Difficulty] section.
	 */
	struct oshu_difficulty difficulty;
	/**
	 * \brief [Event] section.
	 */
	struct oshu_event *events;
	/**
	 * \brief [TimingPoints] section.
	 */
	struct oshu_timing_point *timing_points;
	/**
	 * \brief [Colours] section.
	 */
	struct oshu_color *colors;
	/**
	 * \brief [HitObjects] section.
	 *
	 * It's a linked list of hit objects, in chronological order.
	 */
	struct oshu_hit *hits;
	/**
	 * Pointer to the current hit, according to the context of the game.
	 *
	 * Typically, it would point to the first non-obsolete hit, where a hit
	 * is said to be obsolete when it is nor displayable (not even its
	 * fade-out shadow remains), nor clickable.
	 *
	 * Its goal is to improve the performance of the beatmap drawing
	 * routine and the reactivity on user click, because the obsolete hits
	 * are already skipped.
	 */
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

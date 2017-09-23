/**
 * \file beatmap.h
 * \ingroup beatmap
 */

#pragma once

#include "geometry.h"

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
	OSHU_OSU_MODE = 0,
	OSHU_TAIKO_MODE = 1,
	OSHU_CATCH_THE_BEAT_MODE = 2,
	OSHU_MANIA_MODE = 3,
};

/**
 * Families of sample sets.
 *
 * The constant values are taken from the documentation, and correspond to the
 * value a hit object's addition may mention for its sample set.
 */
enum oshu_sample_set_family {
	/**
	 * The beatmap's hit objects often specifiy 0 as the sample set,
	 * meaning we should use the inherited one.
	 *
	 * However, the parser should in these case replace 0 with the actual
	 * value, so it should be used in an external module.
	 */
	OSHU_AUTO_SAMPLE_SET = 0,
	OSHU_NORMAL_SAMPLE_SET = 1,
	OSHU_SOFT_SAMPLE_SET = 2,
	OSHU_DRUM_SAMPLE_SET = 3,
};

/**
 * Hit sounds to play on top of the normal hit sound.
 *
 * They can be OR'd, so you should store them in an *int*.
 */
enum oshu_hit_sound {
	/**
	 * The first bit is undocumented, let's assume it stands for normal.
	 *
	 * If it's unset, maybe we should play the normal sound? Needs more
	 * research.
	 */
	OSHU_NORMAL_SAMPLE = 1,
	OSHU_WHISTLE_SAMPLE = 2,
	OSHU_FISNISH_SAMPLE = 4,
	OSHU_CLAP_SAMPLE = 8,
};

/**
 * \brief An RGB color.
 *
 * Each field represents a component ranging from 0 to 255 (inclusive).
 */
struct oshu_color {
	int red;
	int green;
	int blue;
};

/**
 * One timing point.
 *
 * This structure is a complete transcription of the timing point description
 * in the documentation, even though it's not used yet.
 *
 * Here's an example: `66,315.789473684211,4,2,0,45,1,0`.
 *
 * The fields of this structures are in the same order as these field-separated
 * values.
 *
 * Some timing points are inherited, and have a negative milleseconds per beat.
 * I guess they should have the inherited flag set too but that's not always
 * the case it seems. These points require us to keep a pointer to the last
 * non-inherited timing point.
 *
 * The structure provides a #next field to make a linked list *in chronological
 * order* with respect to #offset.
 */
struct oshu_timing_point {
	/**
	 * \brief When the timing point starts, in seconds.
	 *
	 * It's an integral number of milliseconds in the beatmap file.
	 */
	double offset;
	/**
	 * \brief Duration of a beat in seconds.
	 *
	 * In the beatmap file, it's a float of the milliseconds per beat.
	 *
	 * When negative, it's a speed multiplier, in percents. The beat
	 * duration of the last non-negative timing point should be multiplied
	 * by this ratio to get the new beat duration. It's crucial for
	 * sliders.
	 */
	double beat_duration;
	/**
	 * \brief Number of beats in a measure.
	 */
	int meter;
	/**
	 * Default sample type to use in that timing section.
	 */
	enum oshu_hit_sound sample_type;
	/**
	 * Default sample set to use in that timing section.
	 */
	enum oshu_sample_set_family sample_set;
	/**
	 * Volume of the samples, from 0 to 100%.
	 *
	 * It's an integer in the beatmap file.
	 */
	double volume;
	/**
	 * Pseudo-boolean telling whether the timing point was inherited or
	 * not. It's looks redundant with the negative #beat_duration.
	 *
	 * Looking at a random beatmap file, I found tons of negative beat
	 * durations but not a single inherited flag. Maybe this flag should
	 * make the parser reuse the sample set, type and volume of the
	 * previous hit point?
	 */
	int inherited;
	/**
	 * Kiai mode. I guess it's when everything flashes on the screen.
	 *
	 * It's a pseudo-boolean, and its position in the timing point line is
	 * unclear. Something tells me it's possibly before the #inherited
	 * field.
	 */
	int kiai;
	/**
	 * Pointer to the next timing point.
	 *
	 * The next point must have a greater #offset.
	 *
	 * NULL for the last item.
	 */
	struct oshu_timing_point *next;
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
	OSHU_HIT_COMBO_OFFSET = 4, /**< How many bits to shift. */
	OSHU_HIT_HOLD = 0b10000000, /**< Mania mode only. */
};

enum oshu_hit_state {
	OSHU_HIT_INITIAL = 0,
	OSHU_HIT_GOOD,
	OSHU_HIT_MISSED,
	OSHU_HIT_SLIDING,
};

/**
 * Parts of a #oshu_hit specific to slider objects.
 *
 * The most complex part of the slider is its path. The way it should be parsed
 * and represented is explained in #oshu_path.
 */
struct oshu_slider {
	/**
	 * Path the slider follows.
	 */
	struct oshu_path path;
	/**
	 * > repeat (Integer) is the number of times a player will go over the
	 * > slider. A value of 1 will not repeat, 2 will repeat once, 3 twice,
	 * > and so on.
	 */
	int repeat;
	/**
	 * Duration of the slider in seconds, one-way without repeat.
	 *
	 * The total duration of the slider is therefore #repeat multiplied by
	 * #duration.
	 *
	 * It is computed from the pixel length in the beatmap file,
	 * #oshu_difficulty::slider_multiplier, and
	 * #oshu_timing_point::beat_duration.
	 */
	double duration;
	/**
	 * Sounds to play on top of the normal sound. One per slider cicle.
	 *
	 * It's an OR'd combination of #oshu_hit_sound.
	 *
	 * \sa edge_additions_set
	 * \sa oshu_hit::hit_sound
	 */
	int edge_hit_sound[2];
	/**
	 * The sample set to use when playing the #edge_hit_sound.
	 *
	 * \sa oshu_hit::additions_set
	 */
	int edge_additions_set[2];
};

/**
 * Parts of a #oshu_hit specific to spinner objects.
 */
struct oshu_spinner {
	/**
	 * Time in seconds when the spinner ends.
	 *
	 * Relative to the song's position, like #oshu_hit::time.
	 */
	double end_time;
};

/**
 * Parts of a #oshu_hit specific to osu!mania hold note objects.
 */
struct oshu_hold_note {
	/**
	 * Time in seconds when the hold note ends.
	 *
	 * Relative to the song's position, like #oshu_hit::time.
	 */
	double end_time;
};

/**
 * One hit object, and one link in the beatmap.
 *
 * This structure actually defines a link in the linked list of all hit
 * objects, sorted by chronological order.
 *
 * It is currently very limited, because it doesn't support sliders, or the
 * sample set.
 *
 * The structure for a hit circle is `x,y,time,type,hitSound,addition`.
 *
 * The structure for a slider is
 * `x,y,time,type,hitSound,sliderType|curvePoints,repeat,pixelLength,edgeHitsounds,edgeAdditions,addition`.
 * See #oshu_slider.
 *
 * The structure for a spinner is `x,y,time,type,hitSound,endTime,addition`.
 * See #oshu_spinner.
 *
 * The structure for a osu!mania hold note is
 * `x,y,time,type,hitSound,endTime:addition`.
 * See #oshu_hold_note.
 *
 * For every type, the addition is structured like
 * `sampleSet:additions:customIndex:sampleVolume:filename`.
 * We'll ignore the filename for now, because it's pretty rare.
 *
 * For details, look at the corresponding field in this structure.
 */
struct oshu_hit {
	/**
	 * Coordinates of the hit object in game coordinates.
	 *
	 * From (0, 0) for top-left to (512, 384) for bottom-right.
	 */
	struct oshu_point p;
	/**
	 * \brief When the hit object should be clicked, in seconds.
	 *
	 * In the beatmap, it's an integral number of milliseconds.
	 */
	double time;
	/**
	 * Type of the hit object, like circle, slider, spinner, and a few
	 * extra information.
	 *
	 * Combination of flags from #oshu_hit_type.
	 */
	int type;
	/**
	 * Combination of flags from #oshu_hit_sound.
	 *
	 * Things to play on top of the normal sound.
	 *
	 * The sample set to use for these additions is defined by the
	 * #additions_set field.
	 */
	int hit_sound;
	/**
	 * Sample set to use when playing the hit sound.
	 *
	 * It's often set to 0 in the beatmap files, but the parser should
	 * replace that value by the sample set to use computed from the
	 * context.
	 *
	 * \sa oshu_timing_point::sample_set
	 */
	enum oshu_sample_set_family sample_set;
	/**
	 * Sample set to use when playing #hit_sound over the normal hit sound,
	 * and not the hit sound itself.
	 *
	 * It's similar to #sample_set.
	 */
	enum oshu_sample_set_family additions_set;
	/**
	 * For a given sample_set family, alternative sample may be used.
	 *
	 * The sample stored in `soft-hitnormal99.wav` may thus be accessed by
	 * setting the sample index to 99.
	 *
	 * By default, it's 1, and means the sample `soft-hitnormal.wav` would
	 * be used instead.
	 */
	int sample_index;
	/**
	 * Volume of the sample, from 0 to 100%.
	 *
	 * \sa #oshu_timing_point::volume
	 */
	double sample_volume;
	/**
	 * Type-specific properties.
	 */
	union {
		struct oshu_slider slider;
		struct oshu_spinner spinner;
		struct oshu_hold_note hold_note;
	};
	/**
	 * Dynamic state of the hit. Whether it was clicked or not.
	 *
	 * It should be left to 0 (#OSHU_HIT_INITIAL) by the parser.
	 */
	enum oshu_hit_state state;
	/**
	 * \brief Timing point in effect when the hit object should be clicked.
	 */
	struct oshu_timing_point *timing_point;
	/**
	 * \brief Combo identifier.
	 *
	 * Starts at 0 at the beginning of the beatmap, and increases at every
	 * hit object with #OSHU_HIT_NEW_COMBO. Its value may increase by more
	 * than 1 if the hit object specifies a non-zero combo skip.
	 */
	int combo;
	/**
	 * \brief Sequence number of the hit inside its combo.
	 *
	 * The first hit object will have the sequence number 1, the next one
	 * 2, and so on until a hit object's type includes #OSHU_HIT_NEW_COMBO,
	 * which resets the sequence number to 1.
	 */
	int combo_seq;
	/**
	 * Pointer to the next element of the linked list.
	 *
	 * NULL if it's the last element.
	 */
	struct oshu_hit *next;
};

/**
 * Tell the time offset, in seconds, when the hit object ends.
 *
 * For a circle, that's the same as #oshu_hit::time, but for a slider, spinner
 * or hold note, it's that offset plus the duration of the hit.
 */
double oshu_hit_end_time(struct oshu_hit *hit);

/**
 * Compute the last point of a hit object.
 *
 * For a circle, it's the same as the starting point, but for a slider it is
 * the position at the end of the slide. If the slider repeats, it may be the
 * same as the starting point though.
 */
struct oshu_point oshu_end_point(struct oshu_hit *hit);

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
	 * \brief Origin of the song.
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
	 * unit. Looks like the bigger it is, the smaller the circles.
	 *
	 * According to `osu.Game.Rulesets.Osu/Objects/OsuHitObject.cs`, we have:
	 *
	 * - OBJECT_RADIUS = 64 (actually, it's the diameter, whatever)
	 * - Scale = (1.0f - 0.7f * (difficulty.CircleSize - 5) / 5) / 2
	 * - Radius = OBJECT_RADIUS * Scale
	 *
	 * Defaults to 32.
	 */
	double circle_radius;
	/**
	 * Number of stars of the song.
	 */
	double overall_difficulty;
	/**
	 * \brief Time tolerance to make a click good, in seconds.
	 *
	 * > OverallDifficulty (Float) specifies the amount of time allowed to
	 * > click a hit object on time.
	 *
	 * It's probably also the number of stars that appear.
	 *
	 * In the beatmap file, it's an obscure float. Let's try to apply some
	 * inverse function, where an *overall difficulty* of 1 yields a
	 * leniency of 0.3 seconds. 3 will be 0.1 seconds, 6 will be 0.05
	 * seconds (getting hard).
	 *
	 * Defaults to 0.1 seconds.
	 *
	 * \sa overall_difficulty
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
	double approach_time;
	/**
	 * Size of the approach circle, in pixels to add to the circle radius.
	 *
	 * The approach circle radius is thus *#circle_radius + r *
	 * #approach_size* where r is a floating number starting at 1 and
	 * decreasing to 0 when the hit object should be hit.
	 *
	 * The property isn't written in the beatmap file, but it's up to the
	 * beatmap loader to fill it with a sane value relative to the circle
	 * size and whatever other variable.
	 */
	double approach_size;
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
	/**
	 * How close the mouse should be to the sliding ball.
	 *
	 * When the player is holding a slider, they should follow the sliding
	 * ball with their mouse. This variable specifies how far the mouse
	 * cursor may be for its position to be considered valid.
	 *
	 * If the distance between the mouse and the ball gets greater than
	 * this slider tolerance, the slider is marked as missed.
	 *
	 * The property isn't written in the beatmap file, but it's up to the
	 * beatmap loader to fill it with a sane value relative to the circle
	 * size and whatever other variable.
	 */
	double slider_tolerance;
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
	 * sample set, so you must check the hit object first before resorting
	 * to this variable.
	 *
	 * Let's default to #OSHU_SAMPLE_SET_SOFT.
	 */
	enum oshu_sample_set_family sample_set;
	/**
	 * The game mode. Today, only the standard osu! game is supported.
	 *
	 * It is written as a number between 0 and 3, and matches the values in
	 * #oshu_mode.
	 */
	enum oshu_mode mode;
	/**
	 * \brief [Metadata] section.
	 */
	struct oshu_metadata metadata;
	/**
	 * \brief [Difficulty] section.
	 */
	struct oshu_difficulty difficulty;
	/**
	 * \brief Path to the background picture.
	 *
	 * It is extracted from the [Events] section, with a line looking like
	 * `0,0,"BG.jpg",0,0`.
	 *
	 * The section may contain other effects and break information, but
	 * until we figure it out, let's keep only this crucial property.
	 *
	 * May be NULL.
	 */
	char *background_file;
	/**
	 * \brief [TimingPoints] section.
	 *
	 * It's a linked list, in chronological order.
	 */
	struct oshu_timing_point *timing_points;
	/**
	 * \brief [Colours] section.
	 *
	 * It's an array owned by the beatmap. Its size is stored in the
	 * #colors_count field.
	 *
	 * These colors are used to display combos, so its index is going to be
	 * a combo identifier modulo the size of this array.
	 */
	struct oshu_color *colors;
	/**
	 * Number of elements in the #colors array.
	 */
	int colors_count;
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
 * Take a path to a `.osu` file, open it and parse it.
 *
 * On failure, the content of *beatmap* is undefined, but any dynamically
 * allocated internal memory is freed.
 */
int oshu_load_beatmap(const char *path, struct oshu_beatmap *beatmap);

/**
 * Free any object dynamically allocated inside the beatmap.
 */
void oshu_destroy_beatmap(struct oshu_beatmap *beatmap);

/** \} */

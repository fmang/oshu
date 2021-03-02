/**
 * \file beatmap/beatmap.h
 * \ingroup beatmap
 */

#pragma once

#include "beatmap/path.h"

namespace oshu {

struct texture;

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
 * osu! website. The .osz files are just ZIP files. After you extract them,
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
enum mode {
	OSU_MODE = 0,
	TAIKO_MODE = 1,
	CATCH_THE_BEAT_MODE = 2,
	MANIA_MODE = 3,
};

/**
 * Families of sample sets.
 *
 * The constant values are taken from the documentation, and correspond to the
 * value a hit object's addition may mention for its sample set.
 *
 * It tends to be called *sample banks* in osu's source code, but as my friend
 * shiro rightfully said, you'd prefer families because families are good while
 * banks are evil.
 */
enum sample_set_family {
	/**
	 * The beatmap's hit objects often specifiy 0 as the sample set,
	 * meaning we should use the inherited one.
	 *
	 * However, the parser should in these case replace 0 with the actual
	 * value, so it should be used in an external module.
	 */
	NO_SAMPLE_SET = -1,
	AUTO_SAMPLE_SET = 0,
	NORMAL_SAMPLE_SET = 1,
	SOFT_SAMPLE_SET = 2,
	DRUM_SAMPLE_SET = 3,
};

/**
 * Types of hit sounds.
 *
 * They're composed of two parts. The low bits store the sound effect, like
 * normal, whistle, finish and clap. The higher bits store the target object
 * type, like hit or slider.
 *
 * They can be OR'd, so you should store them in an *int*.
 */
enum sound_type {
	/**
	 * The first bit is undocumented, but it stands for Normal according to
	 * the source code of osu.
	 *
	 * 0 means None, so I guess it's a silent note.
	 */
	NORMAL_SOUND = 1,
	WHISTLE_SOUND = 2,
	FINISH_SOUND = 4,
	CLAP_SOUND = 8,
	/**
	 * OR this with your sound type to make it a hit sound.
	 *
	 * Note that it's set to 0, and therefore is theoritically optional.
	 */
	HIT_SOUND = 0,
	/**
	 * OR this with your sound type and you'll make it a sliding slider
	 * sound.
	 *
	 * Looks like only oshu::NORMAL_SOUND and oshu::WHISTLE_SOUND may be
	 * combined with these.
	 *
	 * Slider sounds are meant to be looped, unlike hit sounds.
	 *
	 * \todo
	 * Figure out what the sound type 8 means for sliders, because it *is*
	 * used by some beatmaps. It can't be a looping clap, right? Besides,
	 * there's no such file as `sliderclap.wav`.
	 */
	SLIDER_SOUND = 0x80,
	/**
	 * AND this with your combined sound type to retrieve the sound type:
	 * #oshu::NORMAL_SOUND, #oshu::WHISTLE_SOUND, #oshu::FINISH_SOUND,
	 * #oshu::CLAP_SOUND.
	 */
	SOUND_MASK = 0x7F,
	/**
	 * AND this with your combined sound type to retrieve the target hit:
	 * #oshu::HIT_SOUND, #oshu::SLIDER_SOUND.
	 */
	SOUND_TARGET = 0x80,
};

/**
 * \brief An RGB color.
 *
 * Each field represents a component ranging from 0 to 255 (inclusive).
 *
 * It is meant to be used as a *circular* linked list, meaning the last
 * element's next element is the first element. Keep that in mind when looping
 * over it.
 */
struct color {
	/**
	 * The identifier of the combo.
	 *
	 * In a color sequence, indexes must start with 0 and increase by one
	 * without any skip.
	 */
	int index;
	double red;
	double green;
	double blue;
	oshu::color *next;
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
struct timing_point {
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
	 * Default sample set to use in that timing section.
	 */
	enum oshu::sample_set_family sample_set;
	/**
	 * Index of the sample set.
	 *
	 * Like 99 in `normal-hitclap99.wav`.
	 *
	 * This feature is not documented, but from a few beatmaps this is what
	 * I have deduced so far.
	 *
	 * When it's written 0 in the beatmap, it actually means 1.
	 */
	int sample_index;
	/**
	 * Volume of the samples, from 0 to 100%.
	 *
	 * It's an integer in the beatmap file.
	 *
	 * It's stored as a float because the sample format the audio module
	 * uses is also float.
	 */
	float volume;
	/**
	 * Kiai mode. I guess it's when everything flashes on the screen.
	 *
	 * It's a pseudo-boolean, and its position in the timing point line is
	 * unclear.
	 */
	int kiai;
	/**
	 * Pointer to the next timing point.
	 *
	 * The next point must have a greater #offset.
	 *
	 * NULL for the last item.
	 */
	oshu::timing_point *next;
};

/**
 * Flags defining the type of a hit object.
 *
 * To check if it's a circle, you should use `type & oshu::CIRCLE_HIT` rather
 * than check for equality, because it will often be combined with
 * #oshu::NEW_HIT_COMBO.
 */
enum hit_type {
	CIRCLE_HIT = 0b1,
	SLIDER_HIT = 0b10,
	NEW_HIT_COMBO = 0b100,
	SPINNER_HIT = 0b1000,
	COMBO_HIT_MASK = 0b1110000, /**< How many combos to skip. */
	COMBO_HIT_OFFSET = 4, /**< How many bits to shift. */
	HOLD_HIT = 0b10000000, /**< Mania mode only. */
};

/**
 * Transient state of a hit object.
 *
 * It's technically not part of the beatmap but it's really handy. You wouldn't
 * want to maintain a parallel linked list to keep track of the state of every
 * hit.
 */
enum hit_state {
	/**
	 * The default state.
	 *
	 * The hit has never been clicked not skipped nor anything. This also
	 * means the user can interact with the hit object.
	 */
	INITIAL_HIT = 0,
	/**
	 * The slider is currently being held.
	 *
	 * Only meaningful for spinners and mania hold notes.
	 */
	SLIDING_HIT,
	/**
	 * The hit was clicked on time.
	 */
	GOOD_HIT,
	/**
	 * The hit was missed, either because it wasn't clicked at the right
	 * time, or because it wasn't clicked at all.
	 */
	MISSED_HIT,
	/**
	 * A hit obejct may be skipped when the user seeks forward.
	 */
	SKIPPED_HIT,
	/**
	 * Hit objects are marked unknown when the current game mode cannot
	 * interpret it.
	 *
	 * For example, mania hold notes in taiko mode, or spinners in any
	 * other mode than osu. Also spinners in osu mode because they're not
	 * supported yet.
	 */
	UNKNOWN_HIT,
};

/**
 * Structure to encompass all the sound effects information of hit objects.
 *
 * Its fields will look confusing and overlapping, like #index which looks like
 * it applies to both #sample_set and #additions_set, even though they're
 * different sample sets?
 *
 * \todo
 * This structure is too hard to use. The difference between #sample_set and
 * #additions_set is a trap. The #additions field is a combination instead of a
 * simple type. It would be easier to have a structure for 1 sound, and
 * associate a list of sounds to hit objects. The downside is that it would
 * cause a memory indirection, and require more memory. The upside is that the
 * #oshu::hit_sound to filename conversion is easier. As a compromise, if
 * performance matters, the list can be a structure like the current one, with
 * an iterator to convert the compacted multi-sound pack to simple sounds.
 */
struct hit_sound {
	/**
	 * Sample set to use when playing the hit sound.
	 *
	 * It's often set to 0 in the beatmap files, but the parser should
	 * replace that value by the sample set to use, computed from the
	 * context.
	 *
	 * \sa oshu::timing_point::sample_set
	 */
	enum oshu::sample_set_family sample_set;
	/**
	 * Combination of flags from #oshu::sound_type.
	 *
	 * To play the normal sound, #oshu::NORMAL_SOUND must be enabled.
	 *
	 * The sample set to use for these additions is defined by the
	 * #additions_set field, while the normal sound's sample set is defined
	 * by #sample_set.
	 */
	int additions;
	/**
	 * Sample set to use when playing additions over the normal hit sound.
	 *
	 * It's similar to #sample_set.
	 */
	enum oshu::sample_set_family additions_set;
	/**
	 * For a given #sample_set family, alternative samples may be used.
	 *
	 * The sample stored in `soft-hitnormal99.wav` may thus be accessed by
	 * setting the sample index to 99.
	 *
	 * By default, it's 1, and means the sample `soft-hitnormal.wav` would
	 * be used instead.
	 */
	int index;
	/**
	 * Volume of the sample, from 0 to 100%.
	 *
	 * \sa #oshu::timing_point::volume
	 */
	double volume;
};

/**
 * Parts of a #oshu::hit specific to slider objects.
 *
 * The most complex part of the slider is its path. The way it should be parsed
 * and represented is explained in #oshu::path.
 */
struct slider {
	/**
	 * Path the slider follows.
	 */
	oshu::path path;
	/**
	 * > repeat (Integer) is the number of times a player will go over the
	 * > slider. A value of 1 will not repeat, 2 will repeat once, 3 twice,
	 * > and so on.
	 */
	int repeat;
	/**
	 * The length of the slider in osu!pixels.
	 */
	double length;
	/**
	 * Duration of the slider in seconds, one-way without repeat.
	 *
	 * The total duration of the slider is therefore #repeat multiplied by
	 * #duration.
	 *
	 * It is computed from the pixel length in the beatmap file,
	 * #oshu::difficulty::slider_multiplier, and
	 * #oshu::timing_point::beat_duration.
	 */
	double duration;
	/**
	 * Array of sounds to play over each circle.
	 *
	 * #oshu::hit::sound contains the sample for the body of the slider, not
	 * the edges of the slider.
	 *
	 * The size of the array is #repeat + 1. A non-repeating slider will
	 * have 2 sounds. For a repeating slider, it implies the sound for the
	 * same circle will change every time it is repeated.
	 */
	oshu::hit_sound *sounds;
};

/**
 * Parts of a #oshu::hit specific to spinner objects.
 */
struct spinner {
	/**
	 * Time in seconds when the spinner ends.
	 *
	 * Relative to the song's position, like #oshu::hit::time.
	 */
	double end_time;
};

/**
 * Parts of a #oshu::hit specific to osu!mania hold note objects.
 */
struct hold_note {
	/**
	 * Time in seconds when the hold note ends.
	 *
	 * Relative to the song's position, like #oshu::hit::time.
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
 * See #oshu::slider.
 *
 * The structure for a spinner is `x,y,time,type,hitSound,endTime,addition`.
 * See #oshu::spinner.
 *
 * The structure for a osu!mania hold note is
 * `x,y,time,type,hitSound,endTime:addition`.
 * See #oshu::hold_note.
 *
 * For every type, the addition is structured like
 * `sampleSet:additions:customIndex:sampleVolume:filename`.
 * We'll ignore the filename for now, because it's pretty rare.
 *
 * For details, look at the corresponding field in this structure.
 */
struct hit {
	/**
	 * Coordinates of the hit object in game coordinates.
	 *
	 * From (0, 0) for top-left to (512, 384) for bottom-right.
	 */
	oshu::point p;
	/**
	 * \brief When the hit object should be clicked, in seconds.
	 *
	 * In the beatmap, it's an integral number of milliseconds.
	 */
	double time;
	/**
	 * When the hit was clicked by the user, relative to the #time.
	 *
	 * 0 is a perfect hit, -0.5 means the note was hit half a second early.
	 */
	double offset;
	/**
	 * Type of the hit object, like circle, slider, spinner, and a few
	 * extra information.
	 *
	 * Combination of flags from #oshu::hit_type.
	 */
	int type;
	/**
	 * Sound effect to play when the hit object is successfully hit.
	 *
	 * Sliders have some more sounds on the edges, don't forget them.
	 */
	oshu::hit_sound sound;
	/**
	 * Type-specific properties.
	 */
	union {
		oshu::slider slider;
		oshu::spinner spinner;
		oshu::hold_note hold_note;
	};
	/**
	 * \brief Timing point in effect when the hit object should be clicked.
	 *
	 * It is used by the parser to compute the duration of sliders, and may
	 * also be needed by the game or graphics module to handle the slider
	 * ticks, using the #oshu::timing_point::beat_duration property.
	 */
	oshu::timing_point *timing_point;
	/**
	 * \brief Combo identifier.
	 *
	 * Starts at 0 at the beginning of the beatmap, and increases at every
	 * hit object with #oshu::NEW_HIT_COMBO. Its value may increase by more
	 * than 1 if the hit object specifies a non-zero combo skip.
	 *
	 * Two hit objects belong in the same combo if and only if they have
	 * the same combo identifier, unlike #color which wraps.
	 */
	int combo;
	/**
	 * \brief Sequence number of the hit inside its combo.
	 *
	 * The first hit object will have the sequence number 1, the next one
	 * 2, and so on until a hit object's type includes #oshu::NEW_HIT_COMBO,
	 * which resets the sequence number to 1.
	 */
	int combo_seq;
	/**
	 * Color of the hit object, or more specifically, color of the hit's
	 * combo.
	 *
	 * It's closely linked to #combo, and increases in the same way, taking
	 * into account combo skips.
	 */
	oshu::color *color;
	/**
	 * Dynamic state of the hit. Whether it was clicked or not.
	 *
	 * It should be left to 0 (#oshu::INITIAL_HIT) by the parser.
	 */
	enum oshu::hit_state state;
	/**
	 * Graphical texture for the hit object.
	 *
	 * It is up to the game module to decide how to allocate it, draw it,
	 * and free it.
	 *
	 * \todo
	 * The GUI module should manage its own texture cache.
	 */
	oshu::texture *texture;
	/**
	 * Pointer to the previous element of the linked list.
	 *
	 * NULL if it's the first element.
	 */
	oshu::hit *previous;
	/**
	 * Pointer to the next element of the linked list.
	 *
	 * NULL if it's the last element.
	 */
	oshu::hit *next;
};

/**
 * Tell the time offset, in seconds, when the hit object ends.
 *
 * For a circle, that's the same as #oshu::hit::time, but for a slider, spinner
 * or hold note, it's that offset plus the duration of the hit.
 */
double hit_end_time(oshu::hit *hit);

/**
 * Compute the last point of a hit object.
 *
 * For a circle, it's the same as the starting point, but for a slider it is
 * the position at the end of the slide. If the slider repeats, it may be the
 * same as the starting point though.
 */
oshu::point end_point(oshu::hit *hit);

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
struct metadata {
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
 *
 * Looks like 5 in the beatmap file usually means average. Higher than 5 is
 * harder, and lower than 5 is easier.
 */
struct difficulty {
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
	 * Defaults to 32 pixels.
	 */
	double circle_radius;
	/**
	 * Number of stars of the song. It's a nice thing to display.
	 *
	 * > OverallDifficulty (Float) specifies the amount of time allowed to
	 * > click a hit object on time.
	 *
	 * You should not use it for any other purpose. The useful information
	 * it yields should be stored in computed fields, like #leniency.
	 */
	double overall_difficulty;
	/**
	 * \brief Time tolerance to make a click good, in seconds.
	 *
	 * Its value is computed from the #overall_difficulty.
	 *
	 * References:
	 *
	 * - osu.Game.Rulesets.Taiko/Objects/Hit.cs:
	 *     HitWindowMiss = BeatmapDifficulty.DifficultyRange(difficulty.OverallDifficulty, 135, 95, 70);
	 *
	 * - osu.Game/Beatmaps/BeatmapDifficulty.cs:
	 *     if (difficulty > 5) return mid + (max - mid) * (difficulty - 5) / 5;
	 *     if (difficulty < 5) return mid - (mid - min) * (5 - difficulty) / 5;
	 *
	 * - osu.Game.Rulesets.Mania/Judgements/HitWindows.cs:
	 *     private const double miss_min = 316;
	 *     private const double miss_mid = 346;
	 *     private const double miss_max = 376;
	 *     Miss = BeatmapDifficulty.DifficultyRange(difficulty, miss_max, miss_mid, miss_min);
	 *
	 * Defaults to 0.1 seconds.
	 *
	 * \todo
	 * It's currently fixed to 0.1. This should depend on the difficulty settings.
	 *
	 * \todo
	 * For a finer score, there should be several levels of leniency,
	 * mapping to 50, 100 and 150 notes. Look for a video gameplay of Osu!
	 * Tatakae! Ouendan! if you don't get it.
	 */
	double leniency;
	/**
	 * \brief Duration of the approach circle, in seconds.
	 *
	 * > ApproachRate (Float) specifies the amount of time taken for the
	 * > approach circle and hit object to appear.
	 *
	 * Looks like most settings, 1 is easy, 5 is normal, and 9 is hard.
	 *
	 * Defaults to 0.8 seconds.
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
	 * 1 beat is mapped to 100 pixels multiplied by this factor.
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
 * Most string values are dynamically allocated inside this structure. Some
 * linked structures are allocated on the heap too. Make sure you free
 * everything with #oshu::destroy_beatmap.
 */
struct beatmap {
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
	 * Let's default to #oshu::SOFT_SAMPLE_SET.
	 */
	enum oshu::sample_set_family sample_set;
	/**
	 * The game mode. Today, only the standard osu! game is supported.
	 *
	 * It is written as a number between 0 and 3, and matches the values in
	 * #oshu::mode.
	 */
	enum oshu::mode mode;
	/**
	 * \brief [Metadata] section.
	 */
	oshu::metadata metadata;
	/**
	 * \brief [Difficulty] section.
	 */
	oshu::difficulty difficulty;
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
	char *background_filename;
	/**
	 * \brief [TimingPoints] section.
	 *
	 * It's a linked list, in chronological order.
	 */
	oshu::timing_point *timing_points;
	/**
	 * \brief [Colours] section.
	 *
	 * It's a circular linked list, as described in #oshu::color.
	 *
	 * \sa color_count
	 */
	oshu::color *colors;
	/**
	 * Number of colors in the #colors list.
	 */
	int color_count;
	/**
	 * \brief [HitObjects] section.
	 *
	 * It's a linked list of hit objects, in chronological order.
	 *
	 * The list is enclosed by two special unreachable hit objects. All
	 * their fields are zero except the time field which is *-INFINITY* for
	 * the first object, and *+INFINITY* for the last one; and also the
	 * *next* and *previous* pointers. This lets you ensure your hit cursor
	 * is never null.
	 */
	oshu::hit *hits;
};

/**
 * Take a path to a `.osu` file, open it and parse it.
 *
 * On failure, the content of *beatmap* is undefined, but any dynamically
 * allocated internal memory is freed.
 */
int load_beatmap(const char *path, oshu::beatmap *beatmap);

/**
 * Parse the first sections of a beatmap to get the metadata and difficulty
 * information.
 *
 * This function should load the following fields:
 *
 * - #oshu::beatmap::audio_filename,
 * - #oshu::beatmap::background_filename,
 * - #oshu::beatmap::mode,
 * - #oshu::beatmap::metadata,
 * - #oshu::beatmap::difficulty.
 *
 * However, because most fields may be missing from the beatmap, you cannot
 * assume they all have non-NULL values.
 *
 * The aim of this function, compared to #oshu::load_beatmap, is not to load the
 * timing points, colors, and hit objects, which contain most of the beatmap's
 * data.
 */
int load_beatmap_headers(const char *path, oshu::beatmap *beatmap);

/**
 * Free any object dynamically allocated inside the beatmap.
 */
void destroy_beatmap(oshu::beatmap *beatmap);

/**
 * Return a numeric value between 0 (no hits) and 1 (perfect) based on the
 * player's hit/miss ratio.
 */
double score(oshu::beatmap *beatmap);

/** \} */

}

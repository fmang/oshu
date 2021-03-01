/**
 * \file beatmap/parser.cc
 * \ingroup beatmap
 *
 * \brief
 * Beatmap loader.
 */

#include "./parser.h"
#include "beatmap/beatmap.h"
#include "core/log.h"

#include <assert.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>

/**
 * Every osu beatmap file must begin with this.
 */
static const char *osu_file_header = "osu file format v";

/**
 * Structure holding all the default values for a beatmap.
 *
 * This is especially handy for the difficulty section where many values are
 * not 0 by default.
 */
static const oshu::beatmap default_beatmap = {
	.version = 0,
	.audio_filename = nullptr,
	.audio_lead_in = 0,
	.preview_time = 0,
	.countdown = 0,
	.sample_set = oshu::SOFT_SAMPLE_SET,
	.mode = oshu::OSU_MODE,
	.metadata = {},
	.difficulty = {
		.circle_radius = 32.,
		.overall_difficulty = 0,
		.leniency = .1,
		.approach_time = .8,
		.approach_size = 96.,
		.slider_multiplier = 1.4,
		.slider_tick_rate = 1.,
		.slider_tolerance = 64.,
	},
	.background_filename = nullptr,
	.timing_points = nullptr,
	.colors = nullptr,
	.color_count = 0,
	.hits = nullptr,
};

/**
 * Log an error with contextual information from the parser state.
 *
 * It's implemented as a macro because variadic functions are a pain to
 * implement in pure C. Besides, it's something that inlines well. One
 * limitation though: the format string must be a raw string literal.
 *
 * \sa oshu_log_error
 */
#define parser_error(parser, fmt, ...) \
	oshu_log_warning( \
		"%s:%d:%ld: " fmt, \
		(parser)->source, \
		(parser)->line_number, \
		(parser)->input - (parser)->buffer + 1, \
		##__VA_ARGS__ \
	)

using namespace oshu;

/* Primitives ****************************************************************/

static int consume_all(struct parser_state *parser)
{
	parser->input += strlen(parser->input);
	return 0;
}

static int consume_end(struct parser_state *parser)
{
	if (*parser->input == '\0') {
		return 0;
	} else {
		parser_error(parser, "expected end of line, got '%c'", *parser->input);
		return -1;
	}
}

static int consume_spaces(struct parser_state *parser)
{
	while (isspace(*parser->input))
		parser->input++;
	return 0;
}

/**
 * Faster variant of #consume_string for a single character.
 */
static int consume_char(struct parser_state *parser, char c)
{
	if (*parser->input == c) {
		++parser->input;
		return 0;
	} else {
		parser_error(parser, "expected '%c', got '%c'", c, *parser->input);
		return -1;
	}
}

static int consume_string(struct parser_state *parser, const char *str)
{
	size_t len = strlen(str);
	if (strncmp(parser->input, str, len) == 0) {
		parser->input += len;
		return 0;
	} else {
		parser_error(parser, "unexpected input; expected \"%s\"", str);
		return -1;
	}
}

static int parse_char(struct parser_state *parser, char *c)
{
	if (*parser->input == '\0') {
		parser_error(parser, "unexpected end of line");
		return -1;
	}
	*c = *parser->input;
	++parser->input;
	return 0;
}

static int parse_int(struct parser_state *parser, int *value)
{
	char *end;
	*value = strtol(parser->input, &end, 10);
	if (end == parser->input) {
		parser_error(parser, "expected a number");
		return -1;
	} else {
		parser->input = end;
		return 0;
	}
}

/**
 * Parse an int and then consume a separator.
 *
 * This is a convenience function.
 */
static int parse_int_sep(struct parser_state *parser, int *value, char sep)
{
	if (parse_int(parser, value) < 0)
		return -1;
	if (consume_char(parser, sep) < 0)
		return -1;
	return 0;
}

static int parse_double(struct parser_state *parser, double *value)
{
	char *end;
	*value = strtod(parser->input, &end);
	if (end == parser->input) {
		parser_error(parser, "expected a floating number");
		return -1;
	} else {
		parser->input = end;
		return 0;
	}
}

/**
 * \sa parse_int_sep
 */
static int parse_double_sep(struct parser_state *parser, double *value, char sep)
{
	if (parse_double(parser, value) < 0)
		return -1;
	if (consume_char(parser, sep) < 0)
		return -1;
	return 0;
}

/**
 * Parse the remaining of the input as a string.
 *
 * It is not trimmed here. #process_input trims the end of the string, and
 * #parse_key will trim the start for you. Otherwise, call #consume_spaces to
 * trim the start.
 *
 * If the string is empty, `*str` is set to NULL.
 *
 * Otherwise, return a dynamically allocated string using *strdup*, which you
 * must not forget to free!
 */
static int parse_string(struct parser_state *parser, char **str)
{
	if (*parser->input == '\0') {
		*str = NULL;
		return 0;
	} else {
		int len = strlen(parser->input);
		*str = strdup(parser->input);
		parser->input += len;
		return 0;
	}
}

/**
 * Parse a string inside quotes, like `"hello"`.
 *
 * Mainly useful for parsing the file name of the background picture in the
 * events section.
 *
 * Behaves like #parse_string.
 */
static int parse_quoted_string(struct parser_state *parser, char **str)
{
	if (consume_char(parser, '"') < 0) {
		parser_error(parser, "expected double quotes");
		return -1;
	}
	char *end = strchr(parser->input, '"');
	if (!end) {
		parser_error(parser, "unterminated string");
		return -1;
	}
	*end = '\0';
	if (*parser->input == '\0') {
		*str = NULL;
		return 0;
	} else {
		*str = strdup(parser->input);
		parser->input = end + 1;
	}
	return 0;
}

/**
 * Map each token to its string representation using CPP's magic
 * stringification operator.
 */
static const char* token_strings[NUM_TOKENS] = {
#define TOKEN(t) #t,
#include "./tokens.h"
#undef TOKEN
};

/**
 * Perform a dichotomic search using #token_strings.
 *
 * The *str* argument isn't expected to be null-terminated at *len*, so we must
 * make sure we take the length into account when comparing tokens, and not
 * listen too much to *strncmp* which would return 0 if *str* is a prefix of
 * the candidate token.
 */
static int search_token(const char *str, int len, enum token *token)
{
	int start = 0;
	int end = NUM_TOKENS - 1; /* inclusive */
	while (start <= end) {
		int middle = start + (end - start) / 2;
		const char *repr = token_strings[middle];
		int cmp = strncmp(str, repr, len);
		if (cmp == 0)
			cmp = len - strlen(repr);
		if (cmp == 0) {
			/* Found! */
			*token = (enum token) middle;
			return 0;
		} else if (cmp < 0) {
			end = middle - 1;
		} else {
			start = middle + 1;
		}
	}
	return -1;
}

static int parse_token(struct parser_state *parser, enum token *token)
{
	int prefix = 0;
	for (char *c = parser->input; isalpha(*c); ++c)
		++prefix;
	if (prefix == 0) {
		parser_error(parser, "expected an alphabetic token, got '%c'", *parser->input);
		return -1;
	}
	if (search_token(parser->input, prefix, token) < 0) {
		parser_error(parser, "unrecognized token");
		return -1;
	} else {
		parser->input += prefix;
		return 0;
	}
}

static int parse_key(struct parser_state *parser, enum token *key)
{
	if (parse_token(parser, key) < 0)
		return -1;
	consume_spaces(parser);
	if (consume_char(parser, ':') < 0)
		return -1;
	consume_spaces(parser);
	return 0;
}

/*****************************************************************************/
/* New parser ****************************************************************/

static int process_input(struct parser_state *parser)
{
	int rc;
	consume_spaces(parser);
	if (*parser->input == '\0') {
		/* skip empty lines */
		return 0;
	} else if (parser->input[0] == '/' && parser->input[1] == '/') {
		/* skip comments */
		return 0;
	} else if (parser->section == BEATMAP_HEADER) {
		rc = process_header(parser);
		goto end;
	} else if (parser->input[0] == '[') {
		rc = process_section(parser);
		goto end;
	}
	switch (parser->section) {
	case BEATMAP_ROOT:
		parser_error(parser, "unexpected content outside sections");
		return -1;
	case BEATMAP_GENERAL:       rc = process_general(parser); break;
	case BEATMAP_METADATA:      rc = process_metadata(parser); break;
	case BEATMAP_DIFFICULTY:    rc = process_difficulty(parser); break;
	case BEATMAP_EVENTS:        rc = process_event(parser); break;
	case BEATMAP_TIMING_POINTS: rc = process_timing_point(parser); break;
	case BEATMAP_COLOURS:       rc = process_color(parser); break;
	case BEATMAP_HIT_OBJECTS:   rc = process_hit_object(parser); break;
	default:
		/** skip the line */
		rc = consume_all(parser);
	}
end:
	if (rc < 0)
		return -1;
	return consume_end(parser);
}

static int process_header(struct parser_state *parser)
{
	/* skip some binary noise: */
	while (*parser->input && *parser->input != 'o')
		++parser->input;
	if (consume_string(parser, osu_file_header) < 0)
		throw invalid_header();
	if (parse_int(parser, &parser->beatmap->version) < 0)
		throw invalid_header();
	if (parser->beatmap->version < 0)
		throw invalid_header();
	parser->section = BEATMAP_ROOT;
	oshu_log_verbose("beatmap version: %d", parser->beatmap->version);
	return 0;
}

static int process_section(struct parser_state *parser)
{
	enum token token;
	if (consume_char(parser, '[') < 0)
		return -1;
	consume_spaces(parser);
	if (parse_token(parser, &token) < 0)
		return -1;
	consume_spaces(parser);
	if (consume_char(parser, ']') < 0)
		return -1;

	switch (token) {
	case General:
	case Editor:
	case Metadata:
	case Difficulty:
	case Events:
	case TimingPoints:
	case Colours:
	case HitObjects:
		parser->section = (enum beatmap_section) token;
		break;
	default:
		parser_error(parser, "unknown section");
		parser->section = BEATMAP_UNKNOWN;
		return -1;
	}
	if (parser->section == BEATMAP_HIT_OBJECTS)
		validate_colors(parser);
	return 0;
}

static int process_general(struct parser_state *parser)
{
	oshu::beatmap *beatmap = parser->beatmap;
	int rc;
	enum token key;
	int i;
	double d;
	if (parse_key(parser, &key) < 0)
		return -1;
	switch (key) {
	case AudioFilename:
		if ((rc = parse_string(parser, &beatmap->audio_filename)) == 0) {
			if (!beatmap->audio_filename) {
				parser_error(parser, "empty audio file name");
				rc = -1;
			} else if (strchr(beatmap->audio_filename, '/') != NULL) {
				parser_error(parser, "slashes are forbidden in audio file names");
				rc = -1;
			}
		}
		break;
	case AudioLeadIn:
		if ((rc = parse_double(parser, &d)) == 0)
			beatmap->audio_lead_in = d / 1000.;
		break;
	case PreviewTime:
		if ((rc = parse_double(parser, &d)) == 0)
			beatmap->preview_time = d / 1000.;
		break;
	case Countdown:
		if ((rc = parse_int(parser, &i)) == 0)
			beatmap->countdown = i;
		break;
	case Mode:
		if ((rc = parse_int(parser, &i)) == 0)
			beatmap->mode = (oshu::mode) i;
		break;
	case SampleSet:
		rc = parse_sample_set(parser, &beatmap->sample_set);
		break;
	case StackLeniency:
		/* Looks like this is a stat more than something that affects
		 * the gameplay. */
	case LetterboxInBreaks:
		/* From the official doc:
		 * > LetterboxInBreaks (Boolean) specifies whether the
		 * > letterbox appears during breaks.
		 * Very funny. */
	case EpilepsyWarning:
		/* No fear of epilepsy with oshu's current state. */
	case SkinPreference:
		/* Known values so far: `Default`. */
	case StoryFireInFront:
		/* What is this? */
	case EditorBookmarks:
		/* Looks like it contains a list of comma-separated time offsets:
		 * `24271,51160,72273,106051,130051,163829` */
	case EditorDistanceSpacing:
		/* Known values so far: `1.5`. */
	case SpecialStyle:
		/* Known values so far: `0`. */
	case SamplesMatchPlaybackRate:
		/* Known values so far: `0` (probably a pseudo-boolean). */
	case WidescreenStoryboard:
		/* Storyboard are far from being supported. */
		rc = consume_all(parser);
		break;
	default:
		parser_error(parser, "unknown general property");
		return -1;
	}
	return rc;
}

static int parse_sample_set(struct parser_state *parser, enum oshu::sample_set_family *set)
{
	enum token token;
	if (parse_token(parser, &token) < 0)
		return -1;
	switch (token) {
	case Drum:
		*set = oshu::DRUM_SAMPLE_SET;
		break;
	case Normal:
		*set = oshu::NORMAL_SAMPLE_SET;
		break;
	case Soft:
		*set = oshu::SOFT_SAMPLE_SET;
		break;
	case None:
		*set = oshu::NO_SAMPLE_SET;
		break;
	default:
		parser_error(parser, "invalid sample set");
		return -1;
	}
	return 0;
}

static int process_metadata(struct parser_state *parser)
{
	oshu::metadata *meta = &parser->beatmap->metadata;
	enum token key;
	if (parse_key(parser, &key) < 0)
		return -1;
	int rc;
	switch (key) {
	case Title:         rc = parse_string(parser, &meta->title); break;
	case TitleUnicode:  rc = parse_string(parser, &meta->title_unicode); break;
	case Artist:        rc = parse_string(parser, &meta->artist); break;
	case ArtistUnicode: rc = parse_string(parser, &meta->artist_unicode); break;
	case Creator:       rc = parse_string(parser, &meta->creator); break;
	case Version:       rc = parse_string(parser, &meta->version); break;
	case Source:        rc = parse_string(parser, &meta->source); break;
	case Tags:          rc = consume_all(parser); /* TODO */ break;
	case BeatmapID:     rc = parse_int(parser, &meta->beatmap_id); break;
	case BeatmapSetID:  rc = parse_int(parser, &meta->beatmap_set_id); break;
	default:
		parser_error(parser, "unrecognized metadata");
		return -1;
	}
	return rc;
}

static int process_difficulty(struct parser_state *parser)
{
	oshu::difficulty *difficulty = &parser->beatmap->difficulty;
	enum token key;
	double value;
	if (parse_key(parser, &key) < 0)
		return -1;
	if (parse_double(parser, &value) < 0)
		return -1;
	switch (key) {
	case CircleSize:
		difficulty->circle_radius = 54.4 - 4.48 * value;
		assert (difficulty->circle_radius > 0.);
		difficulty->approach_size = 3. * difficulty->circle_radius;
		difficulty->slider_tolerance = 2. * difficulty->circle_radius;
		break;
	case OverallDifficulty:
		difficulty->overall_difficulty = value;
		difficulty->leniency = 0.1 + 0.04 * (5. - value) / 5.;
		assert (difficulty->leniency > 0.);
		break;
	case SliderMultiplier:
		difficulty->slider_multiplier = value;
		break;
	case SliderTickRate:
		difficulty->slider_tick_rate = value;
		break;
	case ApproachRate:
		/* Hardest (10): 0.3s.
		 * Medium  (5):  0.9s.
		 * Easiest (0):  1.5s. */
		difficulty->approach_time = - .12 * value + 1.5;
		assert (difficulty->approach_time > 0.);
		break;
	case HPDrainRate:
		break;
	default:
		parser_error(parser, "unknown difficulty parameter");
		return -1;
	}
	return 0;
}

/**
 * The Events section contains most notably the background image, but also
 * break points.
 *
 * \todo
 * Parse break points. The structure is `2,start,end` where start and end are
 * probably the offset in milliseconds from the beginning of the song. Note
 * however that breaks are automatically detected with the space between two
 * consecutive notes. It's probably not worth handling these until the game is
 * much more mature.
 */
static int process_event(struct parser_state *parser)
{
	if (parser->beatmap->background_filename) {
		/* skip events after we got the background */
		goto skip;
	}
	if (!strncmp(parser->input, "0,0,", 4)) {
		parser->input += 4;
		if (parse_quoted_string(parser, &parser->beatmap->background_filename) < 0)
			return -1;
	}
skip:
	consume_all(parser);
	return 0;
}

/**
 * Parse one timing point and link it.
 *
 * Sample input:
 * `129703,731.707317073171,4,2,1,50,1,0`
 */
static int process_timing_point(struct parser_state *parser)
{
	oshu::timing_point *timing;
	if (parse_timing_point(parser, &timing) < 0)
		return -1;
	if (parser->last_timing_point && timing->offset < parser->last_timing_point->offset) {
		parser_error(parser, "misordered timing point");
		free(timing);
		return -1;
	}
	/* link it to the timing points list */
	if (parser->last_timing_point)
		parser->last_timing_point->next = timing;
	else
		parser->beatmap->timing_points = timing;
	parser->last_timing_point = timing;
	return 0;
}

static int parse_timing_point(struct parser_state *parser, oshu::timing_point **timing)
{
	int value;
	*timing = (oshu::timing_point*) calloc(1, sizeof(**timing));
	assert (timing != NULL);
	/* 0. Defaults. */
	(*timing)->meter = 4;
	(*timing)->sample_set = parser->beatmap->sample_set;
	(*timing)->volume = 1.0;
	/* 1. Timing offset. */
	if (parse_double_sep(parser, &(*timing)->offset, ',') < 0)
		goto fail;
	(*timing)->offset /= 1000.;
	/* 2. Beat duration, in milliseconds. */
	if (parse_double(parser, &(*timing)->beat_duration) < 0)
		goto fail;
	if ((*timing)->beat_duration > 0.) {
		(*timing)->beat_duration /= 1000.;
		parser->timing_base = (*timing)->beat_duration;
	} else if ((*timing)->beat_duration < 0.) {
		if (!parser->timing_base) {
			parser_error(parser, "inherited timing point has no parent");
			goto fail;
		}
		(*timing)->beat_duration = - (*timing)->beat_duration / 100. * parser->timing_base;
	} else {
		parser_error(parser, "invalid beat duration %f", (*timing)->beat_duration);
		goto fail;
	}
	if (*parser->input == '\0')
		goto end;
	else if (consume_char(parser, ',') < 0)
		goto fail;

	/* 3. Number of beats per measure. */
	if (parse_int_sep(parser, &(*timing)->meter, ',') < 0)
		goto fail;
	if ((*timing)->meter <= 0) {
		parser_error(parser, "invalid meter value %d", (*timing)->meter);
		goto fail;
	}
	/* 4. Sample set. */
	if (parse_int_sep(parser, &value, ',') < 0)
		goto fail;
	(*timing)->sample_set = value ? (oshu::sample_set_family) value : parser->beatmap->sample_set;
	/* 5. Looks like this is the sample set index. */
	if (parse_int(parser, &value) < 0)
		goto fail;
	(*timing)->sample_index = value;
	if (*parser->input == '\0')
		goto end;
	else if (consume_char(parser, ',') < 0)
		goto fail;
	/* 6. Volume, from 0 to 100%. */
	if (parse_int_sep(parser, &value, ',') < 0)
		goto fail;
	if (value < 0 || value > 100) {
		parser_error(parser, "invalid volume %d", value);
		goto fail;
	}
	(*timing)->volume = (float) value / 100.;
	/* 7. Inherited flag. Useless? */
	if (parse_int_sep(parser, &value, ',') < 0)
		goto fail;
	/* 8. Kiai mode. */
	if (parse_int(parser, &(*timing)->kiai) < 0)
		goto fail;
end:
	return 0;
fail:
	free(*timing);
	*timing = NULL;
	return -1;
}

static int process_color(struct parser_state *parser)
{
	enum token token;
	if (parse_token(parser, &token) < 0)
		return -1;
	switch (token) {
	case Combo:
		return process_color_combo(parser);
	case SliderBody:
	case SliderTrackOverride:
	case SliderBorder:
		consume_all(parser);
		break;
	default:
		parser_error(parser, "unknown color property");
		return -1;
	}
	return 0;
}

static int process_color_combo(struct parser_state *parser)
{
	/* Check the combo index.
	 * If it's wrong, force it anyway, we cannot affort a weird index space.
	 * It's not even that important. */
	int index;
	if (parse_int(parser, &index) < 0)
		return -1;
	index--;
	if (parser->last_color) {
		if (index != parser->last_color->index + 1)
			parser_error(parser, "suspicious color index");
		else
			index = parser->last_color->index + 1;
	} else if (index != 0) {
		parser_error(parser, "suspicious first color index");
	} else {
		index = 0;
	}

	/* Parse everything else. */
	oshu::color *color;
	consume_spaces(parser);
	if (consume_char(parser, ':') < 0)
		return -1;
	if (parse_color(parser, &color) < 0)
		return -1;
	color->index = index;

	/* Link it. */
	if (parser->last_color)
		parser->last_color->next = color;
	parser->last_color = color;
	if (!parser->beatmap->colors)
		parser->beatmap->colors = color;
	color->next = parser->beatmap->colors;
	assert (parser->beatmap->color_count == index);
	parser->beatmap->color_count = index + 1;
	return 0;
}

static int parse_color(struct parser_state *parser, oshu::color **color)
{
	*color = (oshu::color*) calloc(1, sizeof(**color));
	assert (color != NULL);
	if (parse_color_channel(parser, &(*color)->red) < 0)
		goto fail;
	if (consume_char(parser, ',') < 0)
		goto fail;
	if (parse_color_channel(parser, &(*color)->green) < 0)
		goto fail;
	if (consume_char(parser, ',') < 0)
		goto fail;
	if (parse_color_channel(parser, &(*color)->blue) < 0)
		goto fail;
	return 0;
fail:
	free(*color);
	*color = NULL;
	return -1;
}

static int parse_color_channel(struct parser_state *parser, double *ch)
{
	int v;
	if (parse_int(parser, &v) < 0)
		return -1;
	if (v < 0 || v > 255) {
		parser_error(parser, "color values must be comprised between 0 and 255, inclusive");
		return -1;
	}
	*ch = (double) v / 255.;
	return 0;
}

static void validate_colors(struct parser_state *parser)
{
	if (parser->beatmap->colors)
		return;
	oshu_log_debug("no colors; generating a default color scheme");
	oshu::color *color = (oshu::color*) calloc(1, sizeof(*color));
	color->red = color->green = color->blue = 128;
	color->next = color;
	parser->beatmap->colors = color;
	parser->beatmap->color_count = 1;
}

/*****************************************************************************/
/* Hit objects ***************************************************************/

/**
 * Set the parser's current timing point to the position in seconds specified
 * in *offset*.
 *
 * Return the appropriate object, or NULL on error.
 *
 * The result is stored in #parser_state::current_timing_point for faster
 * results on further calls, assuming you'd never seek to a point before the
 * last seek.
 */
static oshu::timing_point* seek_timing_point(double offset, struct parser_state *parser)
{
	if (parser->current_timing_point == NULL)
		parser->current_timing_point = parser->beatmap->timing_points;
	/* update the parser state as we loop */
	oshu::timing_point **tp = &parser->current_timing_point;
	for (; *tp && (*tp)->next; *tp = (*tp)->next) {
		if ((*tp)->next->offset > offset)
			break;
	}
	return *tp;
}

/**
 * Compute #oshu::hit::combo and #oshu::hit::combo_seq of a single #oshu::hit.
 *
 * Also the update the #oshu::hit::color.
 *
 * The common case for a new combo is to have the #oshu::NEW_HIT_COMBO flag set
 * and the combo skip at 0.
 *
 * \todo
 * Explore the edge cases:
 * - What if the initial hit is marked as a new combo?
 * - What if the initial hit has a combo skip?
 * - What if the combo skip is non-zero but the new combo flag is unset?
 */
static void compute_hit_combo(struct parser_state *parser, oshu::hit *hit)
{
	assert (parser->last_hit != NULL);
	if (parser->last_hit->time < 0.) {
		hit->combo = 0;
		hit->combo_seq = 1;
		hit->color = parser->beatmap->colors;
	} else if (hit->type & oshu::NEW_HIT_COMBO) {
		int skip_combo = (hit->type & oshu::COMBO_HIT_MASK) >> oshu::COMBO_HIT_OFFSET;
		hit->combo = parser->last_hit->combo + 1 + skip_combo;
		hit->combo_seq = 1;
		hit->color = parser->last_hit->color;
		for (int i = 0; hit->color && i < 1 + skip_combo; ++i)
			hit->color = hit->color->next;
	} else {
		hit->combo = parser->last_hit->combo;
		hit->combo_seq = parser->last_hit->combo_seq + 1;
		hit->color = parser->last_hit->color;
	}
	assert (hit->color != NULL);
}

/**
 * The hit's sound additions are parsed after the slider-specific additions,
 * and the slider-specific additions are not complete.
 *
 * Therefore, we need to re-process the slider's sounds after the hit is
 * completely parser.
 */
static void fill_slider_additions(oshu::hit *hit)
{
	assert (hit->type & oshu::SLIDER_HIT);
	for (int i = 0; i <= hit->slider.repeat; ++i) {
		oshu::hit_sound *s = &hit->slider.sounds[i];
		s->additions |= oshu::NORMAL_SOUND;
		if (!s->sample_set)
			s->sample_set = hit->sound.sample_set;
		if (!s->additions_set)
			s->additions_set = hit->sound.additions_set;
		s->index = hit->sound.index;
		s->volume = hit->sound.volume;
	}
}

static int process_hit_object(struct parser_state *parser)
{
	oshu::hit *hit;
	if (parse_hit_object(parser, &hit) < 0)
		return -1;
	compute_hit_combo(parser, hit);
	assert (parser->last_hit != NULL);
	if (hit->time < parser->last_hit->time) {
		parser_error(parser, "missorted hit object");
		free(hit);
		return -1;
	}
	parser->last_hit->next = hit;
	hit->previous = parser->last_hit;
	parser->last_hit = hit;
	return 0;
}

/**
 * Allocate and parse one hit object.
 *
 * On failure, return -1, free any allocated memory, and leave `*hit`
 * unspecified.
 *
 * Consumes:
 * `288,256,8538,2,0,P|254:261|219:255,1,70,8|0,0:0|0:0,0:0:0:0:`
 */
static int parse_hit_object(struct parser_state *parser, oshu::hit **hit)
{
	*hit = (oshu::hit*) calloc(1, sizeof(**hit));
	assert (*hit != NULL);
	if (parse_common_hit(parser, *hit) < 0)
		goto fail;
	(*hit)->timing_point = seek_timing_point((*hit)->time, parser);
	if ((*hit)->timing_point == NULL) {
		parser_error(parser, "could not find the timing point for this hit");
		goto fail;
	}
	if (!((*hit)->type & oshu::CIRCLE_HIT) && consume_char(parser, ',') < 0)
			goto fail;
	int rc;
	if ((*hit)->type & oshu::CIRCLE_HIT) {
		rc = 0;
	} else if ((*hit)->type & oshu::SLIDER_HIT) {
		rc = parse_slider(parser, *hit);
	} else if ((*hit)->type & oshu::SPINNER_HIT) {
		rc = parse_spinner(parser, *hit);
	} else if ((*hit)->type & oshu::HOLD_HIT) {
		rc = parse_hold_note(parser, *hit);
	} else {
		parser_error(parser, "unknown type");
		goto fail;
	}
	if (rc < 0)
		goto fail;
	if (parse_additions(parser, *hit) < 0)
		goto fail;
	if ((*hit)->type & oshu::SLIDER_HIT)
		fill_slider_additions(*hit);
	return 0;
fail:
	free(*hit);
	*hit = NULL;
	return -1;
}

/**
 * Parse the prefix of every hit object.
 *
 * Consumes:
 * `288,256,8538,2,0`
 */
static int parse_common_hit(struct parser_state *parser, oshu::hit *hit)
{
	double x, y;
	if (parse_double_sep(parser, &x, ',') < 0)
		return -1;
	if (parse_double_sep(parser, &y, ',') < 0)
		return -1;
	hit->p = oshu::point{x, y};
	if (parse_double_sep(parser, &hit->time, ',') < 0)
		return -1;
	hit->time /= 1000.;
	if (parse_int_sep(parser, &hit->type, ',') < 0)
		return -1;
	if (parse_int(parser, &hit->sound.additions) < 0)
		return -1;
	hit->sound.additions |= oshu::NORMAL_SOUND;
	if (hit->type & oshu::SLIDER_HIT)
		hit->sound.additions |= oshu::SLIDER_SOUND;
	return 0;
}

/**
 * Parse the specific parts of a slider hit object.
 *
 * Consumes:
 * - `P|396:140|448:80,1,140,0|8,1:0|0:0`
 * - `L|168:88,1,70,8|0,0:0|0:0`
 * - `B|460:188|408:240|408:240|416:280,1,140,4|2,1:2|0:3`
 *
 * Some sliders are shorter and omit the slider additions, like that:
 * `160,76,142685,6,0,B|156:120|116:152,1,70,8|0`
 */
static int parse_slider(struct parser_state *parser, oshu::hit *hit)
{
	char type;
	if (parse_char(parser, &type) < 0)
		return -1;
	if (consume_char(parser, '|') < 0)
		return -1;
	int rc;
	switch (type) {
	case oshu::LINEAR_PATH:  rc = parse_linear_slider(parser, hit); break;
	case oshu::PERFECT_PATH: rc = parse_perfect_slider(parser, hit); break;
	case oshu::CATMULL_PATH: rc = parse_catmull_slider(parser, hit); break;
	case oshu::BEZIER_PATH:  rc = parse_bezier_slider(parser, hit); break;
	default:
		parser_error(parser, "unknown slider type");
		return -1;
	}
	if (rc < 0)
		return -1;
	if (consume_char(parser, ',') < 0)
		return -1;
	if (parse_int_sep(parser, &hit->slider.repeat, ',') < 0)
		return -1;
	if (parse_double(parser, &hit->slider.length) < 0)
		return -1;
	hit->slider.duration = hit->slider.length / (100. * parser->beatmap->difficulty.slider_multiplier) * hit->timing_point->beat_duration;
	oshu::normalize_path(&hit->slider.path, hit->slider.length);
	if (parse_slider_additions(parser, hit) < 0)
		return -1;
	return 0;
}

/**
 * Consumes:
 * `168:88`
 */
static int parse_point(struct parser_state *parser, oshu::point *p)
{
	double x, y;
	if (parse_double_sep(parser, &x, ':') < 0)
		return -1;
	if (parse_double(parser, &y) < 0)
		return -1;
	*p = oshu::point{x, y};
	return 0;
}

static bool parse_slider__more_points(struct parser_state *parser)
{
	if (*parser->input == '|') {
		consume_char(parser, '|');
		return true;
	}
	else
		return false;
}

/**
 * Build a linear path.
 *
 * Consumes:
 * `168:88[|...]`
 *
 */
static int parse_linear_slider(struct parser_state *parser, oshu::hit *hit)
{
	oshu::path *path = &hit->slider.path;
	oshu::point point;
	path->type = oshu::LINEAR_PATH;
	new (&path->line) oshu::line();
	path->line.points.push_back(hit->p);
	do {
		if (parse_point(parser, &point) < 0)
			return -1;
		path->line.points.push_back(point);
	} while (parse_slider__more_points(parser));
	return 0;
}

/**
 * Build a perfect arc.
 *
 * If the points are aligned or something weird, transform it into a linear
 * slider.
 *
 * Consumes:
 * `396:140|448:80'
 */
static int parse_perfect_slider(struct parser_state *parser, oshu::hit *hit)
{
	oshu::point a, b, c;
	a = hit->p;
	if (parse_point(parser, &b) < 0)
		return -1;
	if (consume_char(parser, '|') < 0)
		return -1;
	if (parse_point(parser, &c) < 0)
		return -1;

	hit->slider.path.type = oshu::PERFECT_PATH;
	if (oshu::build_arc(a, b, c, &hit->slider.path.arc) < 0) {
		oshu_log_debug("degenerate perfect arc slider, turning it into a line");
		hit->slider.path.arc.~arc();
		hit->slider.path.type = oshu::LINEAR_PATH;
		new (&hit->slider.path.line) oshu::line();
		hit->slider.path.line.points.push_back(a);
		hit->slider.path.line.points.push_back(c);
	}
	return 0;
}

static int parse_catmull_slider(struct parser_state *parser, oshu::hit *hit)
{
	oshu::path *path = &hit->slider.path;
	oshu::point point;
	path->type = oshu::CATMULL_PATH;
	new (&path->bezier) oshu::bezier();
	path->bezier.control_points.push_back(hit->p);
        do {
		if (parse_point(parser, &point) < 0)
			return -1;
		path->bezier.control_points.push_back(point);
        } while (parse_slider__more_points(parser));
	return 0;
}

/**
 * Parse a Bezier slider.
 *
 * Consumes:
 * `460:188|408:240|408:240|416:280`
 *
 * Segments are cut when a point repeats, as the osu format specifies, but this
 * is a terrible representation. Consider the following path, as it appears in
 * the beatmap: ABBC. There are 2 segments: AB, and BC. Now consider ABBBC. Is
 * it AB+BBC, AB+B+BC, ABB+BC? The naive parsing would do AB+B+BC, which is
 * what's implemented here. This is dumb, but luckily this case is rare enough.
 *
 * More generally, any segment whose control points are all the same points is
 * degenerate, and will be annoying to derive and draw. This requires a special
 * case in the drawing procedure. A smart processing could handle some cases,
 * but there are no right solution since the format is inherently ambiguous.
 *
 * \todo
 * Reclaim the unused memory in the indices aray.
 */
static int parse_bezier_slider(struct parser_state *parser, oshu::hit *hit)
{
	int count = 2;
	for (char *c = parser->input; *c != '\0' && *c != ','; ++c) {
		if (*c == '|')
			count++;
	}

	hit->slider.path.type = oshu::BEZIER_PATH;
	oshu::bezier *bezier = &hit->slider.path.bezier;
	new (bezier) oshu::bezier();
	bezier->control_points.reserve(count);
	bezier->indices.reserve(count);
	bezier->control_points.push_back(hit->p);
	bezier->indices.push_back(0);

	oshu::point prev = bezier->control_points[0];
	for (int i = 1; i < count; i++) {
		if (i > 1 && consume_char(parser, '|') < 0)
			goto fail;
		oshu::point p;
		if (parse_point(parser, &p) < 0)
			goto fail;
		bezier->control_points.push_back(p);
		if (p == prev)
			bezier->indices.push_back(i);
		prev = p;
	}
	bezier->indices.push_back(count);
	bezier->indices.shrink_to_fit();
	return 0;
fail:
	bezier->control_points.clear();
	bezier->indices.clear();
	return -1;
}

/**
 * Parse the slider-specific sound additions, right before the final and common
 * ones.
 *
 * Consumes:
 * `4|2,1:2|0:3
 */
static int parse_slider_additions(struct parser_state *parser, oshu::hit *hit)
{
	hit->slider.sounds = (oshu::hit_sound*) calloc(hit->slider.repeat + 1, sizeof(*hit->slider.sounds));
	assert (hit->slider.sounds != NULL);
	/* Degenerate case. */
	if (*parser->input == '\0')
		return 0;
	else if (consume_char(parser, ',') < 0)
		goto fail;
	/* First field: the hit sounds. */
	for (int i = 0; i <= hit->slider.repeat; ++i) {
		if (i > 0 && consume_char(parser, '|') < 0)
			goto fail;
		if (parse_int(parser, &hit->slider.sounds[i].additions) < 0)
			goto fail;
	}
	/* Second field: the sample sets, for the normal sound and the additions. */
	/* It is optional too, beware! */
	if (*parser->input == '\0')
		return 0;
	else if (consume_char(parser, ',') < 0)
		goto fail;
	for (int i = 0; i <= hit->slider.repeat; ++i) {
		int value;
		if (i > 0 && consume_char(parser, '|') < 0)
			goto fail;
		if (parse_int_sep(parser, &value, ':') < 0)
			goto fail;
		hit->slider.sounds[i].sample_set = (oshu::sample_set_family) value;
		if (parse_int(parser, &value) < 0)
			goto fail;
		hit->slider.sounds[i].additions_set = (oshu::sample_set_family) value;
	}
	return 0;
fail:
	free(hit->slider.sounds);
	hit->slider.sounds = NULL;
	return -1;
}

/**
 * Parse the specific parts of a spinner.
 *
 * Consumes:
 * - `16620`
 */
static int parse_spinner(struct parser_state *parser, oshu::hit *hit)
{
	if (parse_double(parser, &hit->spinner.end_time) < 0)
		return -1;
	hit->spinner.end_time /= 1000.;
	return 0;
}

/**
 * Parse the specific parts of a hold note.
 *
 * Consumes:
 * `16620`
 */
static int parse_hold_note(struct parser_state *parser, oshu::hit *hit)
{
	if (parse_double(parser, &hit->hold_note.end_time) < 0)
		return -1;
	hit->hold_note.end_time /= 1000.;
	return 0;
}

/**
 * Parse the additions, common to every type of note.
 *
 * The whole field is optional.
 *
 * Consumes:
 * - ``
 * - `,0:0:0:0:`
 * - `,1:2:3:100:quack.wav`
 *
 * \todo
 * Store the optional filename at the end when present.
 */
static int parse_additions(struct parser_state *parser, oshu::hit *hit)
{
	/* 0. Fill defaults. */
	hit->sound.sample_set = hit->timing_point->sample_set;
	hit->sound.additions_set = hit->timing_point->sample_set;
	hit->sound.index = hit->timing_point->sample_index;
	hit->sound.volume = hit->timing_point->volume;
	if (*parser->input == '\0')
		return 0;
	if (consume_char(parser, ',') < 0)
		return - 1;
	/* Legacy formats */
	if (*parser->input == '\0')
		return 0;
	int value;
	/* 1. Sample set. */
	if (parse_int(parser, &value) < 0)
		return -1;
	hit->sound.sample_set = value ? (oshu::sample_set_family) value : hit->timing_point->sample_set;
	if (*parser->input != ':')
		return 0;
	consume_char(parser, ':');
	/* 2. Additions set. */
	if (parse_int(parser, &value) < 0)
		return -1;
	hit->sound.additions_set = value ? (oshu::sample_set_family) value : hit->timing_point->sample_set;
	if (*parser->input != ':')
		return 0;
	consume_char(parser, ':');
	/* 3. Custom sample set index. */
	if (parse_int(parser, &value) < 0)
		return -1;
	hit->sound.index = value ? value : hit->timing_point->sample_index;
	if (*parser->input != ':')
		return 0;
	consume_char(parser, ':');
	/* 4. Volume. */
	if (parse_int(parser, &value) < 0)
		return -1;
	if (value < 0 || value > 100) {
		parser_error(parser, "invalid volume %d", value);
		return -1;
	}
	hit->sound.volume = value ? (float) value / 100. : hit->timing_point->volume;
	if (*parser->input != ':')
		return 0;
	consume_char(parser, ':');
	/* 5. File name. */
	/* Unsupported, so we consume everything. */
	consume_all(parser);
	return 0;
}


/*****************************************************************************/
/* Global interface **********************************************************/

/**
 * Create the parser state, then read the input file line-by-line, feeding it
 * to the parser automaton with #process_input.
 *
 * \todo
 * Stop reading the file if the header is incorrect. It's no use printing a
 * mega list of warnings if the file clearly looks nothing like text.
 */
static int parse_file(FILE *input, const char *name, oshu::beatmap *beatmap, bool headers_only)
{
	struct parser_state parser;
	memset(&parser, 0, sizeof(parser));
	parser.section = BEATMAP_HEADER;
	parser.source = name;
	parser.beatmap = beatmap;
	parser.last_hit = beatmap->hits;
	int rc = 0;
	char *line = NULL;
	size_t len = 0;
	ssize_t nread;
	while ((nread = getline(&line, &len, input)) != -1) {
		for (int i = nread - 1; i >= 0 && isspace(line[i]); --i)
			line[i] = '\0';
		parser.buffer = line;
		parser.input = line;
		parser.line_number++;
		try {
			process_input(&parser);
		} catch (invalid_header& e) {
			oshu::error_log() << e.what() << std::endl;
			rc = -1;
			break;
		}
		/* ^ note: ignore parsing errors */
		if (headers_only && parser.section == BEATMAP_TIMING_POINTS)
			break;
	}
	free(line);
	/* Finalize the hits sequence. */
	oshu::hit *end;
	end = (oshu::hit*) calloc(1, sizeof(*end));
	assert (end != NULL);
	end->time = INFINITY;
	parser.last_hit->next = end;
	end->previous = parser.last_hit;
	return rc;
}

/**
 * Initialize the beatmap to its defaults, and add the first unreachable hit
 * object.
 */
void initialize(oshu::beatmap *beatmap)
{
	memcpy(beatmap, &default_beatmap, sizeof(*beatmap));
	beatmap->hits = (oshu::hit*) calloc(1, sizeof(*beatmap->hits));
	assert (beatmap->hits != NULL);
	beatmap->hits->time = -INFINITY;
}

static int validate_metadata(oshu::metadata *meta)
{
	if (!meta->title)
		return -1;
	if (!meta->artist)
		return -1;
	if (!meta->version)
		return -1;
	return 0;
}

/**
 * Perform a variety of checks on a beatmap file to ensure it was parsed well
 * enough to be played.
 */
static int validate(oshu::beatmap *beatmap)
{
	if (!beatmap->audio_filename) {
		oshu_log_error("no audio file mentionned");
		return -1;
	}
	if (validate_metadata(&beatmap->metadata) < 0) {
		oshu_log_error("incomplete metadata");
		return -1;
	}
	return 0;
}

static int load_beatmap(const char *path, oshu::beatmap *beatmap, bool headers_only)

{
	oshu_log_debug("loading beatmap %s", path);
	struct stat s;
	if (stat(path, &s) < 0) {
		oshu_log_error("could not find the beatmap: %s", strerror(errno));
		return -1;
	}
	if (!(S_ISREG(s.st_mode) || S_ISLNK(s.st_mode))) {
		oshu_log_error("not a file: %s", path);
		return -1;
	}
	FILE *input = fopen(path, "r");
	if (input == NULL) {
		oshu_log_error("could not open the beatmap: %s", strerror(errno));
		return -1;
	}
	initialize(beatmap);
	int rc = parse_file(input, path, beatmap, headers_only);
	fclose(input);
	if (rc < 0)
		goto fail;
	if (validate(beatmap) < 0)
		goto fail;
	return 0;
fail:
	oshu_log_error("error loading the beatmap file");
	oshu::destroy_beatmap(beatmap);
	return -1;
}

int oshu::load_beatmap(const char *path, oshu::beatmap *beatmap)
{
	return ::load_beatmap(path, beatmap, false);
}

int oshu::load_beatmap_headers(const char *path, oshu::beatmap *beatmap)
{
	return ::load_beatmap(path, beatmap, true);
}

static void free_metadata(oshu::metadata *meta)
{
	free(meta->title);
	free(meta->title_unicode);
	free(meta->artist);
	free(meta->artist_unicode);
	free(meta->creator);
	free(meta->version);
	free(meta->source);
}

static void free_path(oshu::path *path)
{
	/* Explicitly invoke the destructor. */
	if (path->type == oshu::BEZIER_PATH)
		path->bezier.~bezier();
	else if (path->type == oshu::LINEAR_PATH)
		path->line.~line();
	else if (path->type == oshu::PERFECT_PATH)
		path->arc.~arc();
}

static void free_hits(oshu::hit *hits)
{
	oshu::hit *current = hits;
	while (current != NULL) {
		oshu::hit *next = current->next;
		if (current->type & oshu::SLIDER_HIT) {
			free_path(&current->slider.path);
			free(current->slider.sounds);
		}
		free(current);
		current = next;
	}
}

static void free_timing_points(oshu::timing_point *t)
{
	oshu::timing_point *current = t;
	while (current != NULL) {
		oshu::timing_point *next = current->next;
		free(current);
		current = next;
	}
}

static void free_colors(oshu::color *colors)
{
	if (!colors)
		return;
	oshu::color *current = colors;
	do {
		oshu::color *next = current->next;
		free(current);
		current = next;
	} while (current != colors);
}

void oshu::destroy_beatmap(oshu::beatmap *beatmap)
{
	free(beatmap->audio_filename);
	free(beatmap->background_filename);
	free_metadata(&beatmap->metadata);
	free_timing_points(beatmap->timing_points);
	free_colors(beatmap->colors);
	free_hits(beatmap->hits);
	memset(beatmap, 0, sizeof(*beatmap));
}

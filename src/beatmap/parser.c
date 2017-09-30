/**
 * \file beatmap/parser.c
 * \ingroup beatmap
 *
 * \brief
 * Beatmap loader.
 */

#include "beatmap/beatmap.h"
#include "beatmap/parser.h"
#include "log.h"

#include <assert.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>

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
static const struct oshu_beatmap default_beatmap = {
	.sample_set = OSHU_SOFT_SAMPLE_SET,
	.difficulty = {
		.circle_radius = 32.,
		.leniency = .1,
		.approach_time = .8,
		.approach_size = 96.,
		.slider_multiplier = 1.4,
		.slider_tick_rate = 1.,
		.slider_tolerance = 64.,
	},
};

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
static struct oshu_timing_point* seek_timing_point(double offset, struct parser_state *parser)
{
	if (parser->current_timing_point == NULL)
		parser->current_timing_point = parser->beatmap->timing_points;
	/* update the parser state as we loop */
	struct oshu_timing_point **tp = &parser->current_timing_point;
	for (; *tp && (*tp)->next; *tp = (*tp)->next) {
		if ((*tp)->next->offset > offset)
			break;
	}
	return *tp;
}

/**
 * Compute #oshu_hit::combo and #oshu_hit::combo_seq of a single #oshu_hit.
 *
 * Also the update the #oshu_hit::color.
 */
static void compute_hit_combo(struct parser_state *parser, struct oshu_hit *hit)
{
	if (parser->last_hit == NULL) {
		hit->combo = 0;
		hit->combo_seq = 1;
		hit->color = parser->beatmap->colors;
	} else if (hit->type & OSHU_NEW_HIT_COMBO) {
		int skip_combo = (hit->type & OSHU_COMBO_HIT_MASK) >> OSHU_COMBO_HIT_OFFSET;
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
}

/*****************************************************************************/
/* Old parser ****************************************************************/

/**
 * Trim a string.
 *
 * The passed pointer is moved to the first non-space character, and the
 * trailing spaces are replaced with null characters.
 */
static void trim(char **str)
{
	for (; **str == ' '; (*str)++);
	char *rev = *str + strlen(*str) - 1;
	for (; rev >= *str && *rev == ' '; rev--);
}

/**
 * Parse a point.
 *
 * Sample input:
 * `168:88`
 */
static void build_point(char *line, struct oshu_point *p)
{
	char *x = strsep(&line, ":");
	char *y = line;
	assert (y != NULL);
	p->x = atoi(x);
	p->y = atoi(y);
}

/**
 * Build a linear path.
 *
 * Sample input:
 * `168:88`
 */
static void build_linear_slider(char *line, struct oshu_hit *hit)
{
	struct oshu_path *path = &hit->slider.path;
	path->type = OSHU_LINEAR_PATH;
	path->line.start = hit->p;
	build_point(line, &path->line.end);
}

/**
 * Build a perfect arc.
 *
 * If the points are aligned or something weird, transform it into a linear
 * slider.
 *
 * Sample input:
 * `396:140|448:80'
 */
static void build_perfect_slider(char *line, struct oshu_hit *hit)
{
	hit->slider.path.type = OSHU_PERFECT_PATH;
	char *pass = strsep(&line, "|");
	char *end = line;
	assert (end != NULL);

	struct oshu_point a, b, c;
	a = hit->p;
	build_point(pass, &b);
	build_point(end, &c);

	if (oshu_build_arc(a, b, c, &hit->slider.path.arc) < 0) {
		/* tranform it into a linear path */
		hit->slider.path.type = OSHU_LINEAR_PATH;
		hit->slider.path.line.start = a;
		hit->slider.path.line.end = c;
	}
}

/**
 * Build a Bezier slider.
 *
 * Sample input:
 * `460:188|408:240|408:240|416:280`
 */
static void build_bezier_slider(char *line, struct oshu_hit *hit)
{
	int count = 2;
	for (char *c = line; *c != '\0'; ++c) {
		if (*c == '|')
			count++;
	}

	hit->slider.path.type = OSHU_BEZIER_PATH;
	struct oshu_bezier *bezier = &hit->slider.path.bezier;
	bezier->control_points = calloc(count, sizeof(*bezier->control_points));
	bezier->control_points[0] = hit->p;

	int index = 0;
	bezier->indices = calloc(count, sizeof(*bezier->indices));
	bezier->indices[index] = 0; /* useless but let's remind it */

	struct oshu_point prev = bezier->control_points[0];
	for (int i = 1; i < count; i++) {
		struct oshu_point p;
		char *frag = strsep(&line, "|");
		build_point(frag, &p);
		bezier->control_points[i] = p;
		if (p.x == prev.x && p.y == prev.y) {
			bezier->indices[++index] = i;
		}
		prev = p;
	}
	bezier->indices[++index] = count;
	bezier->segment_count = index;
	oshu_normalize_bezier(bezier);
}


/**
 * Parse the specific parts of a slider hit object.
 *
 * Sample input:
 * - `P|396:140|448:80,1,140,0|8,1:0|0:0,0:0:0:0:`,
 * - `L|168:88,1,70,8|0,0:0|0:0,0:0:0:0:`,
 * - `B|460:188|408:240|408:240|416:280,1,140,4|2,1:2|0:3,0:0:0:0:`.
 */
static void build_slider(char *line, struct parser_state *parser, struct oshu_hit *hit)
{
	char *type = strsep(&line, "|");
	char *path = strsep(&line, ",");
	char *repeat = strsep(&line, ",");
	char *length = strsep(&line, ",");
	assert (length != NULL);
	assert (strlen(type) == 1);
	hit->slider.repeat = atoi(repeat);
	assert (parser->current_timing_point != NULL);
	assert (parser->beatmap->difficulty.slider_multiplier != 0);
	hit->slider.duration = atof(length) / (100. * parser->beatmap->difficulty.slider_multiplier) * parser->current_timing_point->beat_duration;
	if (*type == OSHU_LINEAR_PATH)
		build_linear_slider(path, hit);
	else if (*type == OSHU_PERFECT_PATH)
		build_perfect_slider(path, hit);
	else if (*type == OSHU_BEZIER_PATH)
		build_bezier_slider(path, hit);
	else
		assert(*type != *type);
}

/**
 * Parse specific parts of a spinner.
 */
static void build_spinner(char *line, struct oshu_spinner *spinner)
{
	char *end_time = strsep(&line, ",");
	spinner->end_time = (double) atoi(end_time) / 1000;
}

/**
 * Parse specific parts of a hold note.
 */
static void build_hold_note(char *line, struct oshu_hold_note *hold_note)
{
	char *end_time = strsep(&line, ",");
	hold_note->end_time = (double) atoi(end_time) / 1000;
}

/**
 * Allocate and parse one hit object.
 *
 * On failure, return -1, free any allocated memory, and leave `*hit`
 * unspecified.
 *
 * Sample input:
 * `288,256,8538,2,0,P|254:261|219:255,1,70,8|0,0:0|0:0,0:0:0:0:`
 */
static int build_hit(char *line, struct parser_state *parser, struct oshu_hit **hit)
{
	char *x = strsep(&line, ",");
	char *y = strsep(&line, ",");
	char *time = strsep(&line, ",");
	char *type = strsep(&line, ",");
	char *hit_sound = strsep(&line, ",");
	if (!hit_sound) {
		oshu_log_warning("invalid hit object");
		*hit = NULL;
		return -1;
	}
	*hit = calloc(1, sizeof(**hit));
	(*hit)->p.x = atoi(x);
	(*hit)->p.y = atoi(y);
	(*hit)->time = (double) atoi(time) / 1000;
	(*hit)->type = atoi(type);
	(*hit)->sound.additions = atoi(hit_sound);
	seek_timing_point((*hit)->time, parser);
	if ((*hit)->type & OSHU_SLIDER_HIT)
		build_slider(line, parser, *hit);
	if ((*hit)->type & OSHU_SPINNER_HIT)
		build_spinner(line, &(*hit)->spinner);
	if ((*hit)->type & OSHU_HOLD_HIT)
		build_hold_note(line, &(*hit)->hold_note);
	return 0;
}

/**
 * Parse one hit using #build_hit, and link it into the whole beatmap.
 *
 * Skip invalid hit objects.
 */
static int legacy_parse_hit_object(char *line, struct parser_state *parser)
{
	struct oshu_hit *hit;
	if (build_hit(line, parser, &hit) < 0)
		return 0; /* ignore it */
	if (!parser->beatmap->hits)
		parser->beatmap->hits = hit;
	if (parser->last_hit) {
		assert(parser->last_hit->time <= hit->time);
		parser->last_hit->next = hit;
	}
	compute_hit_combo(parser, hit);
	parser->last_hit = hit;
	return 0;
}

/**
 * The main parser transition function.
 *
 * Dispatch the current line according to the parser state.
 *
 * Trim the lines, and skip comments and empty lines.
 */
static int parse_line(char *line, struct parser_state *parser)
{
	int rc = 0;
	trim(&line);
	if (parser->section == BEATMAP_HIT_OBJECTS) {
		rc = legacy_parse_hit_object(line, parser);
	}
	return rc;
}

/* Primitives ****************************************************************/

static void parser_error(struct parser_state *parser, const char *message)
{
	int column = 0;
	if (parser->buffer && parser->input)
		column = parser->input - parser->buffer + 1;
	oshu_log_error(
		"%s:%d:%d: %s",
		parser->source, parser->line_number, column, message
	);
}

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
		parser_error(parser, "unexpected end of line");
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
		parser_error(parser, "unexpected character");
		oshu_log_error("expected \'%c\'", c);
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
		parser_error(parser, "unexpected input");
		oshu_log_error("expected \"%s\"", str);
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
#include "beatmap/tokens.h"
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
			*token = middle;
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
		parser_error(parser, "expected an alphabetic token");
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
	} else if (parser->input[0] == '[') {
		rc = process_section(parser);
	} else if (parser->section == BEATMAP_ROOT) {
		parser_error(parser, "unexpected content outside sections");
		return -1;
	} else if (parser->section == BEATMAP_GENERAL) {
		rc = process_general(parser);
	} else if (parser->section == BEATMAP_METADATA) {
		rc = process_metadata(parser);
	} else if (parser->section == BEATMAP_DIFFICULTY) {
		rc = process_difficulty(parser);
	} else if (parser->section == BEATMAP_EVENTS) {
		rc = process_event(parser);
	} else if (parser->section == BEATMAP_TIMING_POINTS) {
		rc = process_timing_point(parser);
	} else if (parser->section == BEATMAP_COLOURS) {
		rc = process_color(parser);
	} else {
		return parse_line(parser->input, parser);
	}
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
		return -1;
	if (parse_int(parser, &parser->beatmap->version) < 0)
		return -1;
	if (parser->beatmap->version < 0) {
		parser_error(parser, "invalid osu beatmap version");
		return -1;
	}
	parser->section = BEATMAP_ROOT;
	oshu_log_debug("beatmap version: %d", parser->beatmap->version);
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
		parser->section = token;
		return 0;
	default:
		parser_error(parser, "unknown section");
		parser->section = BEATMAP_UNKNOWN;
		return -1;
	}
}

static int process_general(struct parser_state *parser)
{
	struct oshu_beatmap *beatmap = parser->beatmap;
	int rc;
	enum token key;
	int i;
	double d;
	if (parse_key(parser, &key) < 0)
		return -1;
	switch (key) {
	case AudioFilename:
		if ((rc = parse_string(parser, &beatmap->audio_filename)) == 0) {
			if (strchr(beatmap->audio_filename, '/') != NULL) {
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
			beatmap->mode = i;
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

static int parse_sample_set(struct parser_state *parser, enum oshu_sample_set_family *set)
{
	enum token token;
	if (parse_token(parser, &token) < 0)
		return -1;
	switch (token) {
	case Drum:
		*set = OSHU_DRUM_SAMPLE_SET;
		break;
	case Normal:
		*set = OSHU_NORMAL_SAMPLE_SET;
		break;
	case Soft:
		*set = OSHU_SOFT_SAMPLE_SET;
		break;
	case None:
		*set = OSHU_NO_SAMPLE_SET;
		break;
	default:
		parser_error(parser, "invalid sample set");
		return -1;
	}
	return 0;
}

static int process_metadata(struct parser_state *parser)
{
	struct oshu_metadata *meta = &parser->beatmap->metadata;
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
	struct oshu_difficulty *difficulty = &parser->beatmap->difficulty;
	enum token key;
	double value;
	if (parse_key(parser, &key) < 0)
		return -1;
	if (parse_double(parser, &value) < 0)
		return -1;
	switch (key) {
	case CircleSize:
		difficulty->circle_radius = 32. * (1. - .7 * (value - 5.) / 5.);
		assert (difficulty->circle_radius > 0.);
		difficulty->approach_size = 3. * difficulty->circle_radius;
		difficulty->slider_tolerance = 2. * difficulty->circle_radius;
		break;
	case OverallDifficulty:
		difficulty->overall_difficulty = value;
		break;
	case SliderMultiplier:
		difficulty->slider_multiplier = value;
		break;
	case SliderTickRate:
		difficulty->slider_tick_rate = value;
		break;
	case ApproachRate:
	case HPDrainRate:
		break;
	default:
		parser_error(parser, "unknown difficulty parameter");
		return -1;
	}
	return 0;
}

static int process_event(struct parser_state *parser)
{
	if (!strncmp(parser->input, "0,0,", 4)) {
		parser->input += 4;
		if (parse_quoted_string(parser, &parser->beatmap->background_filename) < 0)
			return -1;
	}
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
	struct oshu_timing_point *timing;
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

static int parse_timing_point(struct parser_state *parser, struct oshu_timing_point **timing)
{
	int value;
	*timing = calloc(1, sizeof(**timing));
	assert (timing != NULL);
	/* 1. Timing offset. */
	if (parse_double_sep(parser, &(*timing)->offset, ',') < 0)
		goto fail;
	(*timing)->offset /= 1000.;
	/* 2. Beat duration, in milliseconds. */
	if (parse_double_sep(parser, &(*timing)->beat_duration, ',') < 0)
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
		parser_error(parser, "invalid beat duration");
		goto fail;
	}
	/* 3. Number of beats per measure. */
	if (parse_int_sep(parser, &(*timing)->meter, ',') < 0)
		goto fail;
	if ((*timing)->meter <= 0) {
		parser_error(parser, "invalid meter value");
		goto fail;
	}
	/* 4. Sample set. */
	if (parse_int_sep(parser, &value, ',') < 0)
		goto fail;
	(*timing)->sample_set = value;
	/* 5. Pretty much like 4, but skipped in the official parser. */
	if (parse_int_sep(parser, &value, ',') < 0)
		goto fail;
	/* 6. Volume, from 0 to 100%. */
	if (parse_int_sep(parser, &value, ',') < 0)
		goto fail;
	if (value < 0 || value > 100) {
		parser_error(parser, "invalid volume");
		goto fail;
	}
	(*timing)->volume = (double) value / 100.;
	/* 7. Inherited flag. Useless? */
	if (parse_int_sep(parser, &value, ',') < 0)
		goto fail;
	/* 8. Kiai mode. */
	if (parse_int(parser, &(*timing)->kiai) < 0)
		goto fail;
	return 0;
fail:
	free(*timing);
	return -1;
}

static int process_color(struct parser_state *parser)
{
	struct oshu_color *color;
	if (parse_color(parser, &color) < 0)
		return -1;
	if (parser->last_color)
		parser->last_color->next = color;
	parser->last_color = color;
	if (!parser->beatmap->colors)
		parser->beatmap->colors = color;
	color->next = parser->beatmap->colors;
	return 0;
}

static int parse_color(struct parser_state *parser, struct oshu_color **color)
{
	int n;
	*color = calloc(1, sizeof(**color));
	assert (color != NULL);
	if (consume_string(parser, "Combo") < 0)
		goto fail;
	if (parse_int(parser, &n) < 0)
		goto fail;
	consume_spaces(parser);
	if (consume_char(parser, ':') < 0)
		goto fail;
	consume_spaces(parser);
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
	return -1;
}

static int parse_color_channel(struct parser_state *parser, int *c)
{
	if (parse_int(parser, c) < 0)
		return -1;
	if (*c < 0 || *c > 255) {
		parser_error(parser, "color values must be comprised between 0 and 255, inclusive");
		return -1;
	}
	return 0;
}

/*****************************************************************************/
/* Hit objects ***************************************************************/

static int process_hit_object(struct parser_state *parser)
{
	struct oshu_hit *hit;
	if (parse_hit_object(parser, &hit) < 0)
		return -1;
	compute_hit_combo(parser, hit);
	if (parser->last_hit) {
		if (hit->time < parser->last_hit->time) {
			parser_error(parser, "missorted hit object");
			free(hit);
			return -1;
		}
		parser->last_hit->next = hit;
		hit->previous = parser->last_hit;
	} else {
		parser->beatmap->hits = hit;
	}
	parser->last_hit = hit;
	return 0;
}

/**
 * Allocate and parse one hit object.
 *
 * On failure, return -1, free any allocated memory, and leave `*hit`
 * unspecified.
 *
 * Sample input:
 * `288,256,8538,2,0,P|254:261|219:255,1,70,8|0,0:0|0:0,0:0:0:0:`
 */
static int parse_hit_object(struct parser_state *parser, struct oshu_hit **hit)
{
	*hit = calloc(1, sizeof(**hit));
	assert (*hit != NULL);
	if (parse_common_hit(parser, *hit) < 0)
		goto fail;
	(*hit)->timing_point = seek_timing_point((*hit)->time, parser);
	if ((*hit)->timing_point == NULL) {
		parser_error(parser, "could not find the timing point for this hit");
		goto fail;
	}
	int rc;
	if ((*hit)->type & OSHU_SLIDER_HIT) {
		rc = parse_slider(parser, *hit);
	} else if ((*hit)->type & OSHU_SPINNER_HIT) {
		rc = parse_spinner(parser, *hit);
	} else if ((*hit)->type & OSHU_HOLD_HIT) {
		rc = parse_hold_note(parser, *hit);
	} else {
		parser_error(parser, "unknown type");
		goto fail;
	}
	if (rc < 0)
		goto fail;
	if (parse_additions(parser, *hit) < 0)
		goto fail;
	return 0;
fail:
	free(*hit);
	return -1;
}

/**
 * Parse the prefix of every hit object.
 *
 * Sample input:
 * `288,256,8538,2,0,`
 */
static int parse_common_hit(struct parser_state *parser, struct oshu_hit *hit)
{
	if (parse_double_sep(parser, &hit->p.x, ',') < 0)
		return -1;
	if (parse_double_sep(parser, &hit->p.y, ',') < 0)
		return -1;
	if (parse_double_sep(parser, &hit->time, ',') < 0)
		return -1;
	hit->time /= 1000.;
	if (parse_int_sep(parser, &hit->type, ',') < 0)
		return -1;
	if (parse_int_sep(parser, &hit->sound.additions, ',') < 0)
		return -1;
	return 0;
}

/**
 * Parse the specific parts of a slider hit object.
 *
 * Sample input:
 * - `P|396:140|448:80,1,140,0|8,1:0|0:0,0:0:0:0:`,
 * - `L|168:88,1,70,8|0,0:0|0:0,0:0:0:0:`,
 * - `B|460:188|408:240|408:240|416:280,1,140,4|2,1:2|0:3,0:0:0:0:`.
 */
static int parse_slider(struct parser_state *parser, struct oshu_hit *hit)
{
	char type;
	if (parse_char(parser, &type) < 0)
		return -1;
	int rc;
	switch (type) {
	case OSHU_LINEAR_PATH:  rc = parse_linear_slider(parser, hit); break;
	case OSHU_PERFECT_PATH: rc = parse_perfect_slider(parser, hit); break;
	case OSHU_BEZIER_PATH:  rc = parse_bezier_slider(parser, hit); break;
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
	double length;
	if (parse_double_sep(parser, &length, ',') < 0)
		return -1;
	hit->slider.duration = length / (100. * parser->beatmap->difficulty.slider_multiplier) * hit->timing_point->beat_duration;
	if (parse_slider_additions(parser, hit) < 0)
		return -1;
	if (consume_char(parser, ',') < 0)
		return -1;
	return 0;
}

/**
 * Sample input:
 * `168:88`
 */
static int parse_point(struct parser_state *parser, struct oshu_point *p)
{
	if (parse_double_sep(parser, &p->x, ':') < 0)
		return -1;
	if (parse_double(parser, &p->y) < 0)
		return -1;
	return 0;
}

/**
 * Build a linear path.
 *
 * sample input:
 * `168:88`
 */
static int parse_linear_slider(struct parser_state *parser, struct oshu_hit *hit)
{
	struct oshu_path *path = &hit->slider.path;
	path->type = OSHU_LINEAR_PATH;
	path->line.start = hit->p;
	if (parse_point(parser, &path->line.end) < 0)
		return -1;
	return 0;
}

/**
 * Build a perfect arc.
 *
 * If the points are aligned or something weird, transform it into a linear
 * slider.
 *
 * Sample input:
 * `396:140|448:80'
 */
static int parse_perfect_slider(struct parser_state *parser, struct oshu_hit *hit)
{
	struct oshu_point a, b, c;
	a = hit->p;
	if (parse_point(parser, &b) < 0)
		return -1;
	if (consume_char(parser, '|') < 0)
		return -1;
	if (parse_point(parser, &c) < 0)
		return -1;

	hit->slider.path.type = OSHU_PERFECT_PATH;
	if (oshu_build_arc(a, b, c, &hit->slider.path.arc) < 0) {
		/* tranform it into a linear path */
		hit->slider.path.type = OSHU_LINEAR_PATH;
		hit->slider.path.line.start = a;
		hit->slider.path.line.end = c;
	}
	return 0;
}

static int parse_bezier_slider(struct parser_state *parser, struct oshu_hit *hit)
{
	return 0;
}

/**
 * Parse the slider-specific sound additions, right before the final and common
 * ones.
 *
 * Sample input:
 * - `4|2,1:2|0:3
 */
static int parse_slider_additions(struct parser_state *parser, struct oshu_hit *hit)
{
	return 0;
}

/**
 * Parse the specific parts of a spinner.
 *
 * Sample input:
 * - `16620`
 */
static int parse_spinner(struct parser_state *parser, struct oshu_hit *hit)
{
	if (parse_double(parser, &hit->spinner.end_time) < 0)
		return -1;
	hit->spinner.end_time /= 1000.;
	return 0;
}

/**
 * Parse the specific parts of a hold note.
 *
 * Sample input:
 * - `16620`
 */
static int parse_hold_note(struct parser_state *parser, struct oshu_hit *hit)
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
 * Sample inputs:
 * - ``
 * - `:0:0:0:0:`
 * - `:1:2:3:100:quack.wav`
 */
static int parse_additions(struct parser_state *parser, struct oshu_hit *hit)
{
	if (*parser->input == '\0') {
		hit->sound.sample_set = hit->timing_point->sample_set;
		hit->sound.additions_set = hit->timing_point->sample_set;
		hit->sound.index = 0;
		hit->sound.volume = 1.;
		return 0;
	}
	int value;
	if (consume_char(parser, ':') < 0)
		return -1;
	/* 1. Sample set. */
	if (parse_int_sep(parser, &value, ':') < 0)
		return -1;
	hit->sound.sample_set = value ? value : hit->timing_point->sample_set;
	/* 2. Additions set. */
	if (parse_int_sep(parser, &value, ':') < 0)
		return -1;
	hit->sound.additions_set = value ? value : hit->timing_point->sample_set;
	/* 3. Custom sample set index. */
	if (parse_int_sep(parser, &hit->sound.index, ':') < 0)
		return -1;
	/* 4. Volume. */
	if (parse_int_sep(parser, &value, ':') < 0)
		return -1;
	hit->sound.volume = value ? value / 100. : 1.;
	/* 5. File name. */
	/* Unsupported, so we consume everything. */
	consume_all(parser);
	return 0;
}


/*****************************************************************************/
/* Global interface **********************************************************/

/**
 * Create the parser state, then read the input file line-by-line, feeding it
 * to the parser automaton with #parse_line.
 */
static int parse_file(FILE *input, const char *name, struct oshu_beatmap *beatmap)
{
	struct parser_state parser;
	memset(&parser, 0, sizeof(parser));
	parser.section = BEATMAP_HEADER;
	parser.source = name;
	parser.beatmap = beatmap;
	char *line = NULL;
	size_t len = 0;
	ssize_t nread;
	while ((nread = getline(&line, &len, input)) != -1) {
		for (int i = nread - 1; i >= 0 && isspace(line[i]); --i)
			line[i] = '\0';
		parser.buffer = line;
		parser.input = line;
		parser.line_number++;
		if (process_input(&parser) < 0) {
			free(line);
			return -1;
		}
	}
	free(line);
	return 0;
}

static void dump_beatmap_info(struct oshu_beatmap *beatmap)
{
	oshu_log_info("audio filename: %s", beatmap->audio_filename);
	oshu_log_debug("slider multiplier: %.1f", beatmap->difficulty.slider_multiplier);
	oshu_log_info("title: %s", beatmap->metadata.title);
	oshu_log_info("artist: %s", beatmap->metadata.artist);
	oshu_log_info("version: %s", beatmap->metadata.version);
}

/**
 * Perform a variety of checks on a beatmap file to ensure it was parsed well
 * enough to be played.
 */
static int validate(struct oshu_beatmap *beatmap)
{
	if (!beatmap->audio_filename) {
		oshu_log_error("no audio file mentionned");
		return -1;
	}
	if (!beatmap->hits) {
		oshu_log_error("no hit objects found");
		return -1;
	}
	return 0;
}

int oshu_load_beatmap(const char *path, struct oshu_beatmap *beatmap)
{
	FILE *input = fopen(path, "r");
	if (input == NULL) {
		oshu_log_error("couldn't open the beatmap: %s", strerror(errno));
		return -1;
	}
	memcpy(beatmap, &default_beatmap, sizeof(*beatmap));
	int rc = parse_file(input, path, beatmap);
	fclose(input);
	if (rc < 0)
		goto fail;
	dump_beatmap_info(beatmap);
	if (validate(beatmap) < 0)
		goto fail;
	return 0;
fail:
	oshu_log_error("error loading the beatmap file");
	oshu_destroy_beatmap(beatmap);
	return -1;
}

static void free_metadata(struct oshu_metadata *meta)
{
	free(meta->title);
	free(meta->title_unicode);
	free(meta->artist);
	free(meta->artist_unicode);
	free(meta->creator);
	free(meta->version);
	free(meta->source);
}

static void free_hits(struct oshu_hit *hits)
{
	struct oshu_hit *current = hits;
	while (current != NULL) {
		struct oshu_hit *next = current->next;
		if (current->type & OSHU_SLIDER_HIT)
			free(current->slider.sounds);
		free(current);
		current = next;
	}
}

static void free_timing_points(struct oshu_timing_point *t)
{
	struct oshu_timing_point *current = t;
	while (current != NULL) {
		struct oshu_timing_point *next = current->next;
		free(current);
		current = next;
	}
}

static void free_colors(struct oshu_color *colors)
{
	if (!colors)
		return;
	struct oshu_color *current = colors;
	do {
		struct oshu_color *next = current->next;
		free(current);
		current = next;
	} while (current != colors);
}

void oshu_destroy_beatmap(struct oshu_beatmap *beatmap)
{
	free(beatmap->audio_filename);
	free(beatmap->background_filename);
	free_metadata(&beatmap->metadata);
	free_timing_points(beatmap->timing_points);
	free_colors(beatmap->colors);
	free_hits(beatmap->hits);
}

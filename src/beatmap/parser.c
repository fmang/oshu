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
		.hp_drain_rate = 1.,
		.circle_radius = 32.,
		.leniency = .1,
		.approach_time = .8,
		.approach_size = 96.,
		.slider_multiplier = 1.4,
		.slider_tick_rate = 1.,
		.slider_tolerance = 64.
	},
};

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
 * Extract a key-value pair from a line like `key: value`.
 *
 * This function receives two pointers to uninitialized pointers, which this
 * function will make point to valid zero-terminated strings. Both the key and
 * the value are trimmed using #trim.
 *
 * If no colon deliminter was found, the whole line is saved into `*key` and
 * `*value` is set to *NULL*.
 */
static void key_value(char *line, char **key, char **value)
{
	char *colon = strchr(line, ':');
	if (colon) {
		*colon = '\0';
		*key = line;
		*value = colon + 1;
		trim(key);
		trim(value);
	} else {
		*key = line;
		*value = NULL;
		trim(key);
	}
}

/**
 * Parse a key-value line in the `[Difficulty]` section.
 */
static int parse_difficulty(char *line, struct parser_state *parser)
{
	char *key, *value;
	key_value(line, &key, &value);
	if (!value)
		return 0;
	if (!strcmp(key, "SliderMultiplier")) {
		parser->beatmap->difficulty.slider_multiplier = atof(value);
	} else if (!strcmp(key, "CircleSize")) {
		double r = (atof(value) - 5.) / 5.;
		parser->beatmap->difficulty.circle_radius = 32. * (1. - .7 * r);
		assert (parser->beatmap->difficulty.circle_radius > 0.);
	}
	return 0;
}

static int parse_event(char *line, struct parser_state *parser)
{
	char *f1 = strsep(&line, ",");
	char *f2 = strsep(&line, ",");
	char *file = strsep(&line, ",");
	if (!file)
		return 0;
	if (!strcmp(f1, "0") && !strcmp(f2, "0")) {
		int len = strlen(file);
		if (file[len-1] == '"')
			file[len-1] = '\0';
		if (file[0] == '"')
			file++;
		parser->beatmap->background_file = strdup(file);
	}
	return 0;
}

static int build_timing_point(char *line, struct parser_state *parser, struct oshu_timing_point **tp)
{
	char *offset = strsep(&line, ",");
	char *beat_length = strsep(&line, ",");
	assert (beat_length != NULL);
	*tp = calloc(1, sizeof(**tp));
	(*tp)->offset = atoi(offset);
	double beat = atof(beat_length);
	if (beat < 0) {
		assert (parser->timing_base != NULL);
		(*tp)->beat_duration = (-beat / 100.) * parser->timing_base->beat_duration;
	} else {
		(*tp)->beat_duration = beat / 1000.;
		parser->timing_base = *tp;
	}
	return 0;
}

/**
 * Parse one timing point.
 *
 * Sample input:
 * `129703,731.707317073171,4,2,1,50,1,0`
 */
static int parse_timing_point(char *line, struct parser_state *parser)
{
	struct oshu_timing_point *tp;
	if (build_timing_point(line, parser, &tp) < 0)
		return 0; /* ignore */
	/* link it to the timing point list */
	if (parser->last_timing_point != NULL) {
		assert (parser->last_timing_point->offset <= tp->offset);
		parser->last_timing_point->next = tp;
	} else {
		parser->beatmap->timing_points = tp;
	}
	parser->last_timing_point = tp;
	return 0;
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
	path->type = OSHU_PATH_LINEAR;
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
	hit->slider.path.type = OSHU_PATH_PERFECT;
	char *pass = strsep(&line, "|");
	char *end = line;
	assert (end != NULL);

	struct oshu_point a, b, c;
	a = hit->p;
	build_point(pass, &b);
	build_point(end, &c);

	if (oshu_build_arc(a, b, c, &hit->slider.path.arc) < 0) {
		/* tranform it into a linear path */
		hit->slider.path.type = OSHU_PATH_LINEAR;
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

	hit->slider.path.type = OSHU_PATH_BEZIER;
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
	if (*type == OSHU_PATH_LINEAR)
		build_linear_slider(path, hit);
	else if (*type == OSHU_PATH_PERFECT)
		build_perfect_slider(path, hit);
	else if (*type == OSHU_PATH_BEZIER)
		build_bezier_slider(path, hit);
	else
		assert(*type != *type);
}

/**
 * Parse specific parts of a spinner.
 */
static void parse_spinner(char *line, struct oshu_spinner *spinner)
{
	char *end_time = strsep(&line, ",");
	spinner->end_time = (double) atoi(end_time) / 1000;
}

/**
 * Parse specific parts of a hold note.
 */
static void parse_hold_note(char *line, struct oshu_hold_note *hold_note)
{
	char *end_time = strsep(&line, ",");
	hold_note->end_time = (double) atoi(end_time) / 1000;
}

/**
 * Set the parser's current timing point to the position in msecs specified in
 * offset.
 */
static void seek_timing_point(int offset, struct parser_state *parser)
{
	struct oshu_timing_point **tp;
	if (parser->current_timing_point == NULL)
		parser->current_timing_point = parser->beatmap->timing_points;
	for (tp = &parser->current_timing_point; *tp && (*tp)->next; *tp = (*tp)->next) {
		if ((*tp)->next->offset > offset)
			break;
	}
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
	if (atoi(type) & OSHU_HIT_SPINNER) {
		oshu_log_debug("skipping spinner");
		return -1;
	}
	*hit = calloc(1, sizeof(**hit));
	(*hit)->p.x = atoi(x);
	(*hit)->p.y = atoi(y);
	(*hit)->time = (double) atoi(time) / 1000;
	(*hit)->type = atoi(type);
	(*hit)->hit_sound = atoi(hit_sound);
	seek_timing_point(atoi(time), parser);
	if ((*hit)->type & OSHU_HIT_SLIDER)
		build_slider(line, parser, *hit);
	if ((*hit)->type & OSHU_HIT_SPINNER)
		parse_spinner(line, &(*hit)->spinner);
	if ((*hit)->type & OSHU_HIT_HOLD)
		parse_hold_note(line, &(*hit)->hold_note);
	return 0;
}

/**
 * Compute #oshu_hit::combo and #oshu_hit::combo_seq of a single #oshu_hit.
 */
static void compute_hit_combo(struct oshu_hit *previous, struct oshu_hit *hit)
{
	if (previous == NULL) {
		hit->combo = 0;
		hit->combo_seq = 1;
	} else if (hit->type & OSHU_HIT_NEW_COMBO) {
		int skip_combo = (hit->type & OSHU_HIT_COMBO_MASK) >> OSHU_HIT_COMBO_OFFSET;
		hit->combo = previous->combo + 1 + skip_combo;
		hit->combo_seq = 1;
	} else {
		hit->combo = previous->combo;
		hit->combo_seq = previous->combo_seq + 1;
	}
}

/**
 * Parse one hit using #build_hit, and link it into the whole beatmap.
 *
 * Skip invalid hit objects.
 */
static int parse_hit_object(char *line, struct parser_state *parser)
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
	compute_hit_combo(parser->last_hit, hit);
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
	if (parser->section == BEATMAP_DIFFICULTY) {
		rc = parse_difficulty(line, parser);
	} else if (parser->section == BEATMAP_EVENTS) {
		rc = parse_event(line, parser);
	} else if (parser->section == BEATMAP_TIMING_POINTS) {
		rc = parse_timing_point(line, parser);
	} else if (parser->section == BEATMAP_HIT_OBJECTS) {
		rc = parse_hit_object(line, parser);
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
	assert (parser->input);
	parser->input += strlen(parser->input);
	return 0;
}

static int consume_end(struct parser_state *parser)
{
	if (!parser->input || *parser->input == '\0') {
		return 0;
	} else {
		parser_error(parser, "unexpected end of line");
		return -1;
	}
}

static int consume_spaces(struct parser_state *parser)
{
	while (parser->input && isspace(*parser->input))
		parser->input++;
	return 0;
}

static int consume_string(struct parser_state *parser, const char *str)
{
	size_t len = strlen(str);
	if (parser->input && strncmp(parser->input, str, len) == 0) {
		parser->input += len;
		return 0;
	} else {
		parser_error(parser, "unexpected input");
		oshu_log_error("expected \"%s\"", str);
		return -1;
	}
}

static int parse_int(struct parser_state *parser, int *value)
{
	if (!parser->input) {
		parser_error(parser, "unexpected end of line");
		return -1;
	}
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

static int parse_double(struct parser_state *parser, double *value)
{
	if (!parser->input) {
		parser_error(parser, "unexpected end of line");
		return -1;
	}
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

static int parse_string(struct parser_state *parser, char **str)
{
	if (!parser->input) {
		parser_error(parser, "unexpected end of line");
		return -1;
	}
	int len = strlen(parser->input);
	for (int i = len - 1; i >= 0 && isspace(parser->input[i]); --i)
		parser->input[i] = '\0';
	if (*parser->input == '\0') {
		parser_error(parser, "expected a string");
		return -1;
	}
	*str = parser->input;
	parser->input += len;
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
	if (!parser->input) {
		parser_error(parser, "unexpected end of line");
		return -1;
	}
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
	if (consume_string(parser, ":") < 0)
		return -1;
	consume_spaces(parser);
	return 0;
}

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
	} else {
		return parse_line(parser->input, parser);
	}
	if (rc < 0)
		return -1;
	consume_spaces(parser);
	return consume_end(parser);
}

static int process_header(struct parser_state *parser)
{
	assert (parser->input);
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
	if (consume_string(parser, "[") < 0)
		return -1;
	consume_spaces(parser);
	if (parse_token(parser, &token) < 0)
		return -1;
	consume_spaces(parser);
	if (consume_string(parser, "]") < 0)
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

struct thing {
	int i;
	double d;
	char *s;
};

static int process_general(struct parser_state *parser)
{
	struct oshu_beatmap *beatmap = parser->beatmap;
	int rc;
	enum token key;
	struct thing value;
	if (parse_key(parser, &key) < 0)
		return -1;
	switch (key) {
	case AudioFilename:
		if ((rc = parse_string(parser, &value.s)) == 0)
			beatmap->audio_filename = strdup(value.s);
		break;
	case AudioLeadIn:
		if ((rc = parse_double(parser, &value.d)) == 0)
			beatmap->audio_lead_in = value.d / 1000.;
		break;
	case PreviewTime:
		if ((rc = parse_double(parser, &value.d)) == 0)
			beatmap->preview_time = value.d / 1000.;
		break;
	case Countdown:
		if ((rc = parse_int(parser, &value.i)) == 0)
			beatmap->countdown = value.i;
		break;
	case Mode:
		if ((rc = parse_int(parser, &value.i)) == 0)
			beatmap->mode = value.i;
		break;
	case SampleSet:
		rc = parse_sample_set(parser, &beatmap->sample_set);
		break;
	case StackLeniency:
	case LetterboxInBreaks:
	case WidescreenStoryboard:
		/** \todo Support these properties. */
		rc = consume_all(parser);
		break;
	default:
		parser_error(parser, "unsupported property");
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
	default:
		parser_error(parser, "invalid sample set");
		return -1;
	}
	return 0;
}

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
		if (nread > 0 && line[nread - 1] == '\n')
			line[nread - 1] = '\0';
		if (nread > 1 && line[nread - 2] == '\r')
			line[nread - 2] = '\0';
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
	} else if (strchr(beatmap->audio_filename, '/') != NULL) {
		oshu_log_error("slashes are forbidden in audio file names");
		return -1;
	}
	if (!beatmap->hits) {
		oshu_log_error("no hit objectings found");
		return -1;
	}
	return 0;
}

int oshu_beatmap_load(const char *path, struct oshu_beatmap **beatmap)
{
	FILE *input = fopen(path, "r");
	if (input == NULL) {
		oshu_log_error("couldn't open the beatmap: %s", strerror(errno));
		return -1;
	}
	*beatmap = malloc(sizeof(**beatmap));
	memcpy(*beatmap, &default_beatmap, sizeof(**beatmap));
	int rc = parse_file(input, path, *beatmap);
	fclose(input);
	if (rc < 0)
		goto fail;
	(*beatmap)->hit_cursor = (*beatmap)->hits;
	dump_beatmap_info(*beatmap);
	if (validate(*beatmap) < 0)
		goto fail;
	return 0;
fail:
	oshu_log_error("error loading the beatmap file");
	oshu_beatmap_free(beatmap);
	return -1;
}

void oshu_beatmap_free(struct oshu_beatmap **beatmap)
{
	if ((*beatmap)->hits) {
		struct oshu_hit *current = (*beatmap)->hits;
		while (current != NULL) {
			struct oshu_hit *next = current->next;
			free(current);
			current = next;
		}
	}
	free((*beatmap)->audio_filename);
	free(*beatmap);
	*beatmap = NULL;
}

/**
 * \file beatmap.c
 * \ingroup beatmap
 */

#include "beatmap.h"
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
	.difficulty = {
		.hp_drain_rate = 1.,
		.circle_radius = 32.,
		.leniency = .1,
		.approach_time = 1.,
		.approach_size = 96.,
		.slider_multiplier = 1.4,
		.slider_tick_rate = 1.,
		.slider_tolerance = 64.
	},
};

/**
 * Enumeration to keep track of the section we're currently in.
 *
 * This is especially important before the format inside each section changes
 * significantly, some being INI-like, and others being CSV-like.
 */
enum beatmap_section {
	BEATMAP_HEADER = 0, /**< Expect the #osu_file_header. */
	BEATMAP_ROOT, /**< Between the header and the first section. We expect nothing. */
	BEATMAP_GENERAL, /**< INI-like. */
	BEATMAP_EDITOR, /**< INI-like. */
	BEATMAP_METADATA, /**< INI-like. */
	BEATMAP_DIFFICULTY, /**< INI-like. */
	BEATMAP_EVENTS, /**< CSV-like. */
	BEATMAP_TIMING_POINTS, /**< CSV-like. */
	BEATMAP_COLOURS, /**< INI-like. */
	BEATMAP_HIT_OBJECTS, /**< CSV-like. */
	BEATMAP_UNKNOWN,
};

/**
 * The parsing process is split into many functions, each minding its own
 * business.
 *
 * To make the prototypes of these functions easy, all they pass in the line to
 * parse and that parser state structure.
 *
 * You'll note the parsing is based on an automaton model, with a transition
 * function receiving one line at a time, and updating the parser state.
 */
struct parser_state {
	struct oshu_beatmap *beatmap;
	enum beatmap_section section;
	struct oshu_hit *last_hit;
	struct oshu_timing_point *last_timing_point;
	struct oshu_timing_point *current_timing_point;
	/** This is the last non-inherited timing point. */
	struct oshu_timing_point *timing_base;
};

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
 * Parse the first line of the beatmap file and extract the version number.
 *
 * Fails if it not a valid header, or if it couldn't parse the version.
 */
static int parse_header(char *line, struct parser_state *parser)
{
	/* skip some binary noise: */
	for (; *line && *line != 'o'; line++);
	if (strncmp(line, osu_file_header, strlen(osu_file_header)) == 0) {
		line += strlen(osu_file_header);
		int version = atoi(line);
		if (version) {
			parser->beatmap->version = version;
			parser->section = BEATMAP_ROOT;
			return 0;
		} else {
			oshu_log_error("invalid osu version");
		}
	} else {
		oshu_log_error("invalid or missing osu file header");
	}
	return -1;
}

/**
 * Parse a section like `[General]`.
 *
 * If the line passed as argument is not a section definition line, or doesn't
 * end with a closing square bracket, fail.
 *
 * Unknown sections are okay.
 */
static int parse_section(char *line, struct parser_state *parser)
{
	char *section, *end;
	if (*line != '[')
		goto fail;
	section = line + 1;
	for (end = section; *end && *end != ']'; end++);
	if (*end != ']')
		goto fail;
	*end = '\0';
	if (!strcmp(section, "General")) {
		parser->section = BEATMAP_GENERAL;
	} else if (!strcmp(section, "Editor")) {
		parser->section = BEATMAP_EDITOR;
	} else if (!strcmp(section, "Metadata")) {
		parser->section = BEATMAP_METADATA;
	} else if (!strcmp(section, "Difficulty")) {
		parser->section = BEATMAP_DIFFICULTY;
	} else if (!strcmp(section, "Events")) {
		parser->section = BEATMAP_EVENTS;
	} else if (!strcmp(section, "TimingPoints")) {
		parser->section = BEATMAP_TIMING_POINTS;
	} else if (!strcmp(section, "Colours")) {
		parser->section = BEATMAP_COLOURS;
	} else if (!strcmp(section, "HitObjects")) {
		parser->section = BEATMAP_HIT_OBJECTS;
	} else {
		oshu_log_debug("unknown section %s", section);
		parser->section = BEATMAP_UNKNOWN;
	}
	return 0;
fail:
	oshu_log_error("misformed section: %s", line);
	return -1;
}

/**
 * Parse a key-value line in the `[General]` section.
 */
static int parse_general(char *line, struct parser_state *parser)
{
	char *key, *value;
	key_value(line, &key, &value);
	if (!value)
		return 0;
	if (!strcmp(key, "AudioFilename")) {
		parser->beatmap->audio_filename = strdup(value);
	} else if (!strcmp(key, "Mode")) {
		parser->beatmap->mode = atoi(value);
	}
	return 0;
}

/**
 * Parse a key-value line in the `[Metadata]` section.
 */
static int parse_metadata(char *line, struct parser_state *parser)
{
	char *key, *value;
	key_value(line, &key, &value);
	if (!value)
		return 0;
	if (!strcmp(key, "Title"))
		parser->beatmap->metadata.title = strdup(value);
	else if (!strcmp(key, "TitleUnicode"))
		parser->beatmap->metadata.title_unicode = strdup(value);
	else if (!strcmp(key, "Artist"))
		parser->beatmap->metadata.artist = strdup(value);
	else if (!strcmp(key, "ArtistUnicode"))
		parser->beatmap->metadata.artist_unicode = strdup(value);
	else if (!strcmp(key, "Creator"))
		parser->beatmap->metadata.creator = strdup(value);
	else if (!strcmp(key, "Version"))
		parser->beatmap->metadata.version = strdup(value);
	else if (!strcmp(key, "Source"))
		parser->beatmap->metadata.source = strdup(value);
	else if (!strcmp(key, "Tags")){
		parser->beatmap->metadata._tags = strdup(value);
		int tags_nbr = 2;
		int len = -1;
		while (parser->beatmap->metadata._tags[++len]){
			if (parser->beatmap->metadata._tags[len] == ' '){
				parser->beatmap->metadata._tags[len] = '\0';
				tags_nbr++;
			}
		}
		parser->beatmap->metadata.tags = malloc(sizeof(char *) * tags_nbr);
		if (!parser->beatmap->metadata.tags)
			return 0;
		parser->beatmap->metadata.tags[--tags_nbr] = NULL;
		while (len-- >= 0){
			if (!parser->beatmap->metadata._tags[len]){
				tags_nbr--;
				parser->beatmap->metadata.tags[tags_nbr] =\
				 &parser->beatmap->metadata._tags[len + 1];
			}
		}
	}
	else if (!strcmp(key, "BeatmapID"))
		parser->beatmap->metadata.beatmap_id = atoi(value);
	else if (!strcmp(key, "BeatmapSetID"))
		parser->beatmap->metadata.beatmap_set_id = atoi(value);
	return 0;
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
	bezier->lengths = calloc(bezier->segment_count, sizeof(*bezier->lengths));
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
		oshu_log_warn("invalid hit object");
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
	if (*line == '\0') {
		/* skip empty lines */
	} else if (line[0] == '/' && line[1] == '/') {
		/* skip comments */
	} else if (parser->section == BEATMAP_HEADER) {
		rc = parse_header(line, parser);
	} else if (*line == '[') {
		rc = parse_section(line, parser);
	} else if (parser->section == BEATMAP_GENERAL) {
		rc = parse_general(line, parser);
	} else if (parser->section == BEATMAP_METADATA) {
		rc = parse_metadata(line, parser);
	} else if (parser->section == BEATMAP_DIFFICULTY) {
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

/**
 * Create the parser state, then read the input file line-by-line, feeding it
 * to the parser automaton with #parse_line.
 */
static int parse_file(FILE *input, struct oshu_beatmap *beatmap)
{
	struct parser_state parser;
	memset(&parser, 0, sizeof(parser));
	parser.beatmap = beatmap;
	char *line = NULL;
	size_t len = 0;
	ssize_t nread;
	while ((nread = getline(&line, &len, input)) != -1) {
		if (nread > 0 && line[nread - 1] == '\n')
			line[nread - 1] = '\0';
		if (nread > 1 && line[nread - 2] == '\r')
			line[nread - 2] = '\0';
		if (parse_line(line, &parser) < 0) {
			free(line);
			return -1;
		}
	}
	free(line);
	return 0;
}

static void dump_beatmap_info(struct oshu_beatmap *beatmap)
{
	oshu_log_info("beatmap version: %d", beatmap->version);
	oshu_log_info("audio filename: %s", beatmap->audio_filename);
	oshu_log_info("Song title: %s", beatmap->metadata.title);
	oshu_log_info("Song artist: %s", beatmap->metadata.artist);
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
	int rc = parse_file(input, *beatmap);
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

double oshu_hit_end_time(struct oshu_hit *hit)
{
	if (hit->type & OSHU_HIT_SLIDER && hit->slider.path.type)
		return hit->time + hit->slider.duration * hit->slider.repeat;
	else
		return hit->time;
}

struct oshu_point oshu_end_point(struct oshu_hit *hit)
{
	if (hit->type & OSHU_HIT_SLIDER && hit->slider.path.type)
		return oshu_path_at(&hit->slider.path, hit->slider.repeat);
	else
		return hit->p;
}

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
		.approach_rate = 1.,
		.slider_multiplier = 1.4,
		.slider_tick_rate = 1.,
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

static int build_timing_point(char *line, struct parser_state *parser, struct oshu_timing_point **tp)
{
	char *offset = strsep(&line, ",");
	char *beat_length = strsep(&line, ",");
	assert (beat_length != NULL);
	*tp = calloc(1, sizeof(**tp));
	(*tp)->offset = atoi(offset);
	double beat = atof(beat_length);
	if (beat < 0) {
		assert (parser->last_timing_point != NULL);
		(*tp)->beat_duration = parser->last_timing_point->beat_duration;
	} else {
		(*tp)->beat_duration = beat / 1000.;
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
 * Allocate and parse one hit object.
 *
 * On failure, return -1, free any allocated memory, and leave `*hit`
 * unspecified.
 *
 * Sample input:
 * `288,256,8538,2,0,P|254:261|219:255,1,70,8|0,0:0|0:0,0:0:0:0:`
 */
static int parse_one_hit(char *line, struct oshu_hit **hit)
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
	*hit = calloc(1, sizeof(**hit));
	(*hit)->x = atoi(x);
	(*hit)->y = atoi(y);
	(*hit)->time = (double) atoi(time) / 1000;
	(*hit)->type = atoi(type);
	(*hit)->hit_sound = atoi(hit_sound);
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
 * Parse one hit using #parse_one_hit, and link it into the whole beatmap.
 *
 * Skip invalid hit objects.
 */
static int parse_hit_object(char *line, struct parser_state *parser)
{
	struct oshu_hit *hit;
	if (parse_one_hit(line, &hit) < 0)
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

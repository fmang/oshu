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
 * Enumeration to keep track of the section we're currently in.
 *
 * This is especially important before the format inside each section changes
 * significantly, some being INI-like, and others being CSV-like.
 */
enum beatmap_section {
	BEATMAP_HEADER = 0, /**< Expect the #osu_file_header. */
	BEATMAP_ROOT, /**< Between the header and the first section. We expect nothing. */
	BEATMAP_GENERAL, /**< INI-like. */
	BEATMAP_METADATA, /**< INI-like. */
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
	} else if (!strcmp(section, "Metadata")) {
		parser->section = BEATMAP_METADATA;
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
 * Allocate and parse one hit object.
 *
 * Only failure, `*hit` is set to *NULL*.
 */
static void parse_one_hit(char *line, struct oshu_hit **hit)
{
	char *x = strsep(&line, ",");
	char *y = strsep(&line, ",");
	char *time = strsep(&line, ",");
	char *type = strsep(&line, ",");
	if (!type) {
		oshu_log_warn("invalid hit object");
		*hit = NULL;
		return;
	}
	*hit = calloc(1, sizeof(**hit));
	(*hit)->x = atoi(x);
	(*hit)->y = atoi(y);
	(*hit)->time = (double) atoi(time) / 1000;
	(*hit)->type = atoi(type);
}

/**
 * Parse one hit using #parse_one_hit, and link it into the whole beatmap.
 *
 * Skip invalid hit objects.
 */
static int parse_hit_object(char *line, struct parser_state *parser)
{
	struct oshu_hit *hit;
	parse_one_hit(line, &hit);
	if (hit) {
		if (!parser->beatmap->hits)
			parser->beatmap->hits = hit;
		if (parser->beatmap->hit_cursor) {
			assert(parser->beatmap->hit_cursor->time < hit->time);
			parser->beatmap->hit_cursor->next = hit;
		}
		parser->beatmap->hit_cursor = hit;
	}
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
	*beatmap = calloc(1, sizeof(**beatmap));
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

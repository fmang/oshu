#include "oshu.h"

#include <stdio.h>
#include <string.h>

static const char *osu_file_header = "\xef\xbb\xbfosu file format v";

enum beatmap_section {
	BEATMAP_HEADER = 0,
	BEATMAP_ROOT,
	BEATMAP_GENERAL,
	BEATMAP_METADATA,
	BEATMAP_UNKNOWN,
};

struct parser_state {
	struct oshu_beatmap *beatmap;
	enum beatmap_section section;
};

static void trim(char **str)
{
	for (; **str == ' '; (*str)++);
	char *rev = *str + strlen(*str) - 1;
	for (; rev >= *str && *rev == ' '; rev--);
}

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

static int parse_header(char *line, struct parser_state *parser)
{
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
	} else {
		oshu_log_debug("unknown section %s", section);
		parser->section = BEATMAP_UNKNOWN;
	}
	return 0;
fail:
	oshu_log_error("misformed section: %s", line);
	return -1;
}

static int parse_general(char *line, struct parser_state *parser)
{
	char *key, *value;
	key_value(line, &key, &value);
	if (!value)
		return 0;
	if (!strcmp(key, "AudioFilename")) {
		parser->beatmap->general.audio_filename = strdup(value);
	} else if (!strcmp(key, "Mode")) {
		parser->beatmap->general.mode = atoi(value);
	}
	return 0;
}

static int parse_line(char *line, struct parser_state *parser)
{
	int rc = 0;
	/* skip spaces */
	for (; *line == ' '; line++);
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
	}
	return rc;
}

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
	oshu_log_info("audio filename: %s", beatmap->general.audio_filename);
}

int oshu_beatmap_load(const char *path, struct oshu_beatmap **beatmap)
{
	FILE *input = fopen(path, "r");
	if (input == NULL) {
		oshu_log_error("couldn't open the beatmap: %s", strerror(errno));
		return -1;
	}
	*beatmap = calloc(1, sizeof(**beatmap));
	if (parse_file(input, *beatmap) < 0) {
		oshu_log_error("error parsing the beatmap file");
		fclose(input);
		oshu_beatmap_free(beatmap);
		return -1;
	}
	dump_beatmap_info(*beatmap);
	return 0;
}

void oshu_beatmap_free(struct oshu_beatmap **beatmap)
{
	free((*beatmap)->general.audio_filename);
	free(*beatmap);
	*beatmap = NULL;
}

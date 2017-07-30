#include "oshu.h"

#include <stdio.h>
#include <string.h>

#define OSU_FILE_HEADER "osu file format v"

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

int parse_header(char *line, struct parser_state *parser)
{
	if (strncmp(line, OSU_FILE_HEADER, strlen(OSU_FILE_HEADER)) == 0) {
		line += strlen(OSU_FILE_HEADER);
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

int parse_line(char *line, struct parser_state *parser)
{
	/* skip spaces */
	for (; *line == ' '; line++);
	/* skip empty lines */
	if (*line == '\0' || *line == '\n')
		return 0;
	if (parser->section == BEATMAP_HEADER) {
		if (parse_header(line, parser) < 0)
			return -1;
	} else if (*line == '[') {
		parser->section = BEATMAP_UNKNOWN;
	}
	return 0;
}

int parse_file(FILE *input, struct oshu_beatmap *beatmap)
{
	struct parser_state parser;
	memset(&parser, 0, sizeof(parser));
	parser.beatmap = beatmap;
	char *line = NULL;
	size_t len = 0;
	ssize_t nread;
	while ((nread = getline(&line, &len, input)) != -1) {
		if (parse_line(line, &parser) < 0) {
			free(line);
			return -1;
		}
	}
	free(line);
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
	if (parse_file(input, *beatmap) < 0) {
		fclose(input);
		oshu_beatmap_free(beatmap);
		return -1;
	}
	return 0;
}

void oshu_beatmap_free(struct oshu_beatmap **beatmap)
{
	free(*beatmap);
	*beatmap = NULL;
}

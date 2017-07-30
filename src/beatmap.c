#include "oshu.h"

#include <stdio.h>

enum beatmap_section {
	BEATMAP_HEADER,
	BEATMAP_GENERAL,
	BEATMAP_METADATA,
	BEATMAP_UNKNOWN,
};

struct parser_state {
	struct oshu_beatmap *beatmap;
	enum beatmap_section section;
};

int parse_line(char *line, struct parser_state *parser)
{
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

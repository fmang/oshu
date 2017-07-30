#include "oshu.h"

int oshu_beatmap_load(const char *path, struct oshu_beatmap **beatmap)
{
	*beatmap = calloc(1, sizeof(**beatmap));
	return 0;
}

void oshu_beatmap_free(struct oshu_beatmap **beatmap)
{
	free(*beatmap);
	*beatmap = NULL;
}

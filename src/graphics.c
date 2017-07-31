#include "oshu.h"

int oshu_display_init(struct oshu_display **display)
{
	*display = calloc(1, sizeof(**display));
	return 0;
}

void oshu_display_destroy(struct oshu_display **display)
{
	free(*display);
	*display = NULL;
}

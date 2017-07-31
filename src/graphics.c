#include "oshu.h"

static const int window_width = 640; /* px */
static const int window_height = 480; /* px */

int oshu_display_init(struct oshu_display **display)
{
	*display = calloc(1, sizeof(**display));
	(*display)->window = SDL_CreateWindow(
		"Oshu!",
		SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
		window_width, window_height,
		0
	);
	if ((*display)->window == NULL)
		goto fail;
	(*display)->renderer = SDL_CreateRenderer((*display)->window, -1, 0);
	if ((*display)->renderer == NULL)
		goto fail;
	return 0;
fail:
	oshu_log_error("error creating the display: %s", SDL_GetError());
	oshu_display_destroy(display);
	return -1;
}

void oshu_display_destroy(struct oshu_display **display)
{
	if ((*display)->renderer)
		SDL_DestroyRenderer((*display)->renderer);
	if ((*display)->window)
		SDL_DestroyWindow((*display)->window);
	free(*display);
	*display = NULL;
}

void oshu_draw_beatmap(struct oshu_display *display, struct oshu_beatmap *beatmap, int msecs)
{
	SDL_SetRenderDrawColor(display->renderer, 0, 0, 0, 255);
	SDL_RenderClear(display->renderer);
	SDL_RenderPresent(display->renderer);
}

/**
 * \file graphics/display.c
 * \ingroup display
 */

#include "graphics/display.h"
#include "log.h"

static const int game_width = 640; /* px */
static const int game_height = 480; /* px */

int create_window(struct oshu_display *display)
{
	display->window = SDL_CreateWindow(
		"oshu!",
		SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
		game_width, game_height,
		SDL_WINDOW_RESIZABLE
	);
	if (display->window == NULL)
		goto fail;
	display->renderer = SDL_CreateRenderer(display->window, -1, 0);
	if (display->renderer == NULL)
		goto fail;
	return 0;
fail:
	oshu_log_error("error creating the display: %s", SDL_GetError());
	return -1;
}

int load_textures(struct oshu_display *display)
{
	return 0;
}

int oshu_open_display(struct oshu_display **display)
{
	*display = calloc(1, sizeof(**display));
	(*display)->viewport.zoom = 1.;
	if (create_window(*display) < 0)
		goto fail;
	if (load_textures(*display) < 0)
		goto fail;
	return 0;
fail:
	oshu_close_display(display);
	return -1;
}

void oshu_close_display(struct oshu_display **display)
{
	if ((*display)->renderer)
		SDL_DestroyRenderer((*display)->renderer);
	if ((*display)->window)
		SDL_DestroyWindow((*display)->window);
	free(*display);
	*display = NULL;
}

void oshu_resize_display(struct oshu_display *display)
{
	int w, h;
	SDL_GetWindowSize(display->window, &w, &h);
	double original_ratio = (double) game_width / game_height;
	double actual_ratio = (double) w / h;
	if (actual_ratio > original_ratio) {
		/* the window is too wide */
		display->viewport.zoom = (double) h / game_height;
		display->viewport.left = (w - game_width * display->viewport.zoom) / 2.;
		display->viewport.top = 0.;
	} else {
		/* the window is too high */
		display->viewport.zoom = (double) w / game_width;
		display->viewport.left = 0.;
		display->viewport.top = (h - game_height * display->viewport.zoom) / 2.;
	}
}

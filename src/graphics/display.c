/**
 * \file graphics/display.c
 * \ingroup display
 */

#include "graphics/display.h"
#include "log.h"

#include <assert.h>
#include <SDL2/SDL.h>

/**
 * Open the window and create the rendered.
 *
 * The default window size, 960x720 is arbitrary but proportional the the game
 * area. It's just a saner default for most screens.
 */
int create_window(struct oshu_display *display)
{
	SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "linear");
	display->window = SDL_CreateWindow(
		"oshu!",
		SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
		960, 720,
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

int oshu_open_display(struct oshu_display *display)
{
	if (create_window(display) < 0)
		goto fail;
	oshu_reset_view(display);
	return 0;
fail:
	oshu_close_display(display);
	return -1;
}

void oshu_close_display(struct oshu_display *display)
{
	if (display->renderer) {
		SDL_DestroyRenderer(display->renderer);
		display->renderer = NULL;
	}
	if (display->window) {
		SDL_DestroyWindow(display->window);
		display->window = NULL;
	}
}

oshu_point oshu_get_mouse(struct oshu_display *display)
{
	int x, y;
	SDL_GetMouseState(&x, &y);
	return oshu_unproject(display, x + y * I);
}

/**
 * \file graphics/display.c
 * \ingroup display
 */

#include "graphics/display.h"
#include "log.h"

#include <assert.h>

/*
 * These two constants are crucial for correctly defining the viewport.
 * They're not arbitrary and required by osu's beatmap format.
 */
static const int game_width = 640; /* px */
static const int game_height = 480; /* px */

/**
 * Open the window and create the rendered.
 *
 * The default window size, 960x720 is arbitrary but proportional the the game
 * area. It's just a saner default for most screens.
 */
int create_window(struct oshu_display *display)
{
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
		display->viewport.x = (w - game_width * display->viewport.zoom) / 2.;
		display->viewport.y = 0.;
	} else {
		/* the window is too high */
		display->viewport.zoom = (double) w / game_width;
		display->viewport.x = 0.;
		display->viewport.y = (h - game_height * display->viewport.zoom) / 2.;
	}
}

struct oshu_point oshu_get_mouse(struct oshu_display *display)
{
	int x, y;
	SDL_GetMouseState(&x, &y);
	return oshu_unproject(display, (struct oshu_point) {x, y});
}

struct oshu_point oshu_project(struct oshu_display *display, struct oshu_point p)
{
	switch (display->system) {
	case OSHU_GAME_COORDINATES:
		p.x += 64.;
		p.y += 48.;
	case OSHU_VIEWPORT_COORDINATES:
		p.x = (p.x * display->viewport.zoom) + display->viewport.x;
		p.y = (p.y * display->viewport.zoom) + display->viewport.y;
	case OSHU_WINDOW_COORDINATES:
		break;
	default:
		assert (display->system != display->system);
	}
	return p;
}

struct oshu_point oshu_unproject(struct oshu_display *display, struct oshu_point p)
{
	switch (display->system) {
	case OSHU_WINDOW_COORDINATES:
		break;
	case OSHU_VIEWPORT_COORDINATES:
	case OSHU_GAME_COORDINATES:
		p.x = (p.x - display->viewport.x) / display->viewport.zoom;
		p.y = (p.y - display->viewport.y) / display->viewport.zoom;
		if (display->system == OSHU_GAME_COORDINATES) {
			p.x -= 64.;
			p.y -= 48.;
		}
		break;
	default:
		assert (display->system != display->system);
	}
	return p;
}

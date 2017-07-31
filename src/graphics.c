#include "oshu.h"

#include <SDL2/SDL_image.h>

static const int window_width = 640; /* px */
static const int window_height = 480; /* px */

static const int hit_radius = 32; /* px */

int create_window(struct oshu_display *display)
{
	display->window = SDL_CreateWindow(
		"Oshu!",
		SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
		window_width, window_height,
		0
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
	display->hit_mark = IMG_LoadTexture(display->renderer, "hit.png");
	if (display->hit_mark == NULL) {
		oshu_log_error("error load the textures: %s", SDL_GetError());
		return -1;
	}
	return 0;
}

int oshu_display_init(struct oshu_display **display)
{
	*display = calloc(1, sizeof(**display));
	if (create_window(*display) < 0)
		goto fail;
	if (load_textures(*display) < 0)
		goto fail;
	return 0;
fail:
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
	if (beatmap->hit_cursor) {
		SDL_Rect where;
		where.x = beatmap->hit_cursor->x - hit_radius;
		where.y = beatmap->hit_cursor->y - hit_radius;
		where.w = hit_radius * 2;
		where.h = hit_radius * 2;
		SDL_RenderCopy(display->renderer, display->hit_mark, NULL, &where);
	}
	SDL_RenderPresent(display->renderer);
}

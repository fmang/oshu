/**
 * \file ui/background.c
 * \ingroup ui_background
 */

#include "ui/background.h"

#include "graphics/display.h"

#include <assert.h>
#include <SDL2/SDL.h>

int oshu_load_background(struct oshu_display *display, const char *filename, struct oshu_background *background)
{
	background->display = display;
	return oshu_load_texture(display, filename, &background->picture);
}

/**
 * Draw a background image on the entire screen.
 *
 * Scale the texture to the window's size, while preserving the aspect ratio.
 * When the aspects don't match, crop the picture to ensure the window is
 * filled.
 */
static void fill_screen(struct oshu_display *display, struct oshu_texture *pic)
{
	SDL_Rect dest;
	int ww, wh;
	SDL_GetWindowSize(display->window, &ww, &wh);
	double window_ratio = (double) ww / wh;
	double pic_ratio = oshu_ratio(pic->size);

	if (window_ratio > pic_ratio) {
		/* the window is too wide */
		dest.w = ww;
		dest.h = dest.w / pic_ratio;
		dest.x = 0;
		dest.y = (wh - dest.h) / 2;
	} else {
		/* the window is too high */
		dest.h = wh;
		dest.w = dest.h * pic_ratio;
		dest.y = 0;
		dest.x = (ww - dest.w) / 2;
	}
	SDL_RenderCopy(display->renderer, pic->texture, NULL, &dest);
}

void oshu_show_background(struct oshu_background *background, double brightness)
{
	if (!background->picture.texture)
		return;
	assert (brightness >= 0);
	assert (brightness <= 1);
	int mod = 64 + brightness * 191;
	assert (mod >= 0);
	assert (mod <= 255);
	SDL_SetTextureColorMod(background->picture.texture, mod, mod, mod);
	fill_screen(background->display, &background->picture);
}

void oshu_destroy_background(struct oshu_background *background)
{
	oshu_destroy_texture(&background->picture);
}

double oshu_trapezium(double start, double end, double transition, double t)
{
	double ratio = 0.;
	if (t <= start)
		ratio = 0.;
	else if (t < start + transition)
		ratio = (t - start) / transition;
	else if (t <= end - transition)
		ratio = 1.;
	else if (t < end)
		ratio = (end - t) / transition;
	else
		ratio = 0.;
	return ratio;
}

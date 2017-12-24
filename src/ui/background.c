/**
 * \file ui/background.c
 * \ingroup ui_background
 */

#include "ui/background.h"

#include "graphics/display.h"
#include "log.h"

#include <assert.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>

/**
 * Adjust the background size such that it fits the display.
 *
 * The result is written in *dest*, such that the rectangle covers the whole
 * window while possibly being cropped.
 */
static void fit(struct oshu_display *display, oshu_size size, SDL_Rect *dest)
{
	oshu_size vsize = display->view.size;
	double window_ratio = oshu_ratio(vsize);;
	double pic_ratio = oshu_ratio(size);

	if (window_ratio > pic_ratio) {
		/* the window is too wide */
		dest->w = creal(vsize);
		dest->h = dest->w / pic_ratio;
		dest->x = 0;
		dest->y = (cimag(vsize) - dest->h) / 2;
	} else {
		/* the window is too high */
		dest->h = cimag(vsize);
		dest->w = dest->h * pic_ratio;
		dest->y = 0;
		dest->x = (creal(vsize) - dest->w) / 2;
	}
}

/**
 * Downscale the background for the display, if necessary.
 *
 * When scaling is performed, the input surface is freed and the pointer made
 * point to the new scaled surface.
 *
 * \todo
 * Handle blit errors.
 */
static int scale_background(struct oshu_display *display, SDL_Surface **pic)
{
	SDL_Rect target_rect;
	fit(display, (*pic)->w + I * (*pic)->h, &target_rect);
	if (target_rect.w >= (*pic)->w)
		return 0; /* don't upscale */

	SDL_Surface *target = SDL_CreateRGBSurfaceWithFormat(0, target_rect.w, target_rect.h, 24, SDL_PIXELFORMAT_RGB888);
	SDL_BlitScaled(*pic, NULL, target, NULL);
	SDL_FreeSurface(*pic);
	*pic = target;
	return 0;
}

int oshu_load_background(struct oshu_display *display, const char *filename, struct oshu_background *background)
{
	memset(background, 0, sizeof(*background));
	SDL_Surface *pic = IMG_Load(filename);
	if (!pic) {
		oshu_log_error("error loading background: %s", IMG_GetError());
		return -1;
	}
	if (scale_background(display, &pic) < 0)
		return -1;

	background->display = display;
	background->picture.size = pic->w + I * pic->h;
	background->picture.origin = 0;
	background->picture.texture = SDL_CreateTextureFromSurface(display->renderer, pic);
	SDL_FreeSurface(pic);

	if (!background->picture.texture) {
		oshu_log_error("error uploading background: %s", SDL_GetError());
		return -1;
	} else {
		return 0;
	}
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
	fit(display, pic->size, &dest);
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

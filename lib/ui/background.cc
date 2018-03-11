/**
 * \file ui/background.cc
 * \ingroup ui_background
 *
 * \todo
 * The image loader uses SDL2_image, and then toys with cairo to resize it.
 * Instead, we could use ImageMagick to do both a once. In the near future,
 * we'll require ImageMagick anyway to generate thumbnails. Let's use that
 * opportunity to delete the SDL2_image dependency too.
 */

#include "ui/background.h"

#include "graphics/display.h"
#include "core/log.h"

#include <assert.h>
#include <cairo/cairo.h>
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
		dest->w = std::real(vsize);
		dest->h = dest->w / pic_ratio;
		dest->x = 0;
		dest->y = (std::imag(vsize) - dest->h) / 2;
	} else {
		/* the window is too high */
		dest->h = std::imag(vsize);
		dest->w = dest->h * pic_ratio;
		dest->y = 0;
		dest->x = (std::real(vsize) - dest->w) / 2;
	}
}

/**
 * Downscale the background for the display, if necessary.
 *
 * When scaling is performed, the input surface is freed and the pointer made
 * point to the new scaled surface.
 *
 * First, the resulting image size is computing using the same function as
 * #oshu_show_background. If the resulting size is smaller than the original
 * size, do nothing.
 *
 * Then, the image is converted to unpacked RGB: each pixel is made of 4-bytes,
 * one of which is unused. This format is called SDL_PIXELFORMAT_RGB888, or
 * CAIRO_FORMAT_RGB24, but *not* SDL_PIXELFORMAT_RGB24. Cairo doesn't seem to
 * support packed 3-byte RGB.
 *
 * The destination SDL surface is created with the wanted size. Both surfaces
 * are then converted to cairo surfaces, without memory duplication because
 * that's how cairo integrates with SDL best. The source is painted on the
 * destination.
 *
 * Finally, everything is unlocked and freed except the destination surface,
 * which is stored in `*pic`. Voilà!
 *
 * It might sound a bit overkill to use cairo for that, but oshu! already
 * depends on it anyway, and the best SDL provides is SDL_BlitScaled, which
 * albeit fast, ruins the quality.
 *
 * \todo
 * Handle cairo errors.
 */
static int scale_background(struct oshu_display *display, SDL_Surface **pic)
{
	SDL_Rect target_rect;
	fit(display, oshu_size((*pic)->w, (*pic)->h), &target_rect);
	if (target_rect.w >= (*pic)->w)
		return 0; /* don't upscale */
	double zoom = (double) target_rect.w / (*pic)->w;
	oshu_log_debug("scaling the background to %dx%d (%.1f%%)", target_rect.w, target_rect.h, zoom * 100);

	if ((*pic)->format->format != SDL_PIXELFORMAT_RGB888) {
		oshu_log_debug("converting the background picture to unpacked RGB");
		SDL_Surface *converted = SDL_ConvertSurfaceFormat(*pic, SDL_PIXELFORMAT_RGB888, 0);
		assert (converted != NULL);
		SDL_FreeSurface(*pic);
		*pic = converted;
	}
	SDL_Surface *target = SDL_CreateRGBSurfaceWithFormat(0, target_rect.w, target_rect.h, 24, SDL_PIXELFORMAT_RGB888);
	assert ((*pic)->format->format == SDL_PIXELFORMAT_RGB888);
	assert (target->format->format == SDL_PIXELFORMAT_RGB888);

	SDL_LockSurface(target);
	SDL_LockSurface(*pic);
	cairo_surface_t *source = cairo_image_surface_create_for_data(
		(unsigned char*) (*pic)->pixels, CAIRO_FORMAT_RGB24,
		(*pic)->w, (*pic)->h, (*pic)->pitch);
	cairo_surface_t *dest = cairo_image_surface_create_for_data(
		(unsigned char*) target->pixels, CAIRO_FORMAT_RGB24,
		target->w, target->h, target->pitch);

	cairo_t *cr = cairo_create(dest);
	cairo_scale(cr, zoom, zoom);
	cairo_set_source_surface(cr, source, 0, 0);
	cairo_paint(cr);
	cairo_destroy(cr);

	cairo_surface_destroy(dest);
	cairo_surface_destroy(source);
	SDL_UnlockSurface(*pic);
	SDL_UnlockSurface(target);
	SDL_FreeSurface(*pic);
	*pic = target;
	return 0;
}

int oshu_load_background(struct oshu_display *display, const char *filename, struct oshu_background *background)
{
	memset(background, 0, sizeof(*background));
	background->display = display;
	if (!(display->features & OSHU_SHOW_BACKGROUND))
		return 0;

	SDL_Surface *pic = IMG_Load(filename);
	if (!pic) {
		oshu_log_error("error loading background: %s", IMG_GetError());
		return -1;
	}
	if (scale_background(display, &pic) < 0)
		return -1;

	background->picture.size = oshu_size(pic->w, pic->h);
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
	if (!background->display) /* uninitialized background */
		return;
	if (!(background->display->features & OSHU_SHOW_BACKGROUND))
		return;
	oshu_destroy_texture(&background->picture);
}

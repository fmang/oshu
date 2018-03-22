/**
 * \file video/paint.cc
 * \ingroup video_paint
 */

#include "video/paint.h"

#include "video/display.h"
#include "video/texture.h"
#include "core/log.h"

#include <assert.h>
#include <SDL2/SDL.h>

static void destroy_painter(struct oshu_painter *painter)
{
	if (painter->cr) {
		cairo_destroy(painter->cr);
		painter->cr = NULL;
	}
	if (painter->surface) {
		cairo_surface_destroy(painter->surface);
		painter->surface = NULL;
	}
	if (painter->destination) {
		SDL_FreeSurface(painter->destination);
		painter->destination = NULL;
	}
}

int oshu_start_painting(struct oshu_display *display, oshu_size size, struct oshu_painter *painter)
{
	cairo_status_t s;
	memset(painter, 0, sizeof(*painter));
	painter->display = display;
	painter->size = size;

	double zoom = display->view.zoom;
	size *= zoom;

	/* 1. SDL */
	painter->destination = SDL_CreateRGBSurface(
		0, std::real(size), std::imag(size), 32,
		0x00FF0000, 0x0000FF00, 0x000000FF, 0xFF000000);
	if (!painter->destination) {
		oshu_log_error("could not create a painting surface: %s", SDL_GetError());
		goto fail;
	}
	if (SDL_LockSurface(painter->destination) < 0) {
		oshu_log_error("could not lock the surface: %s", SDL_GetError());
		goto fail;
	}

	/* 2. Cairo surface */
	painter->surface = cairo_image_surface_create_for_data(
		(unsigned char*) painter->destination->pixels, CAIRO_FORMAT_ARGB32,
		std::real(size), std::imag(size), painter->destination->pitch);
	s = cairo_surface_status(painter->surface);
	if (s != CAIRO_STATUS_SUCCESS) {
		oshu_log_error("cairo surface error: %s", cairo_status_to_string(s));
		goto fail;
	}

	/* 3. Cairo context */
	painter->cr = cairo_create(painter->surface);
	s = cairo_status(painter->cr);
	if (s != CAIRO_STATUS_SUCCESS) {
		oshu_log_error("cairo error: %s", cairo_status_to_string(s));
		goto fail;
	}
	cairo_scale(painter->cr, zoom, zoom);

	return 0;

fail:
	destroy_painter(painter);
	return -1;
}

/**
 * Cairo uses pre-multiplied alpha channels.
 *
 * What this means is that a bright red (0xFF0000) at alpha 50% is stored as
 * 0x800000. We need to divide by the alpha to restore the initial color.
 *
 * This is required for Cairo → SDL interoperability.
 */
static void unpremultiply(SDL_Surface *surface)
{
	uint8_t *pixels = (uint8_t*) surface->pixels;
	assert (surface->pitch % 4 == 0);
	assert (surface->pitch == 4 * surface->w);
	uint8_t *end = pixels + surface->h * surface->pitch;
	for (uint8_t *c = pixels; c < end; c += 4) {
		uint8_t alpha = c[3];
		if (alpha == 0)
			continue;
		c[0] = (unsigned int) c[0] * 255 / alpha;
		c[1] = (unsigned int) c[1] * 255 / alpha;
		c[2] = (unsigned int) c[2] * 255 / alpha;
	}
}

int oshu_finish_painting(struct oshu_painter *painter, struct oshu_texture *texture)
{
	int rc = 0;
	unpremultiply(painter->destination);
	SDL_UnlockSurface(painter->destination);
	texture->size = painter->size;
	texture->origin = 0;
	texture->texture = SDL_CreateTextureFromSurface(painter->display->renderer, painter->destination);
	if (!texture->texture) {
		oshu_log_error("error uploading texture: %s", SDL_GetError());
		rc = -1;
	}
	destroy_painter(painter);
	return rc;
}

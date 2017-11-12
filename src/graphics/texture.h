/**
 * \file graphics/texture.h
 * \ingroup graphics_texture
 */

#pragma once

#include "beatmap/geometry.h"

struct SDL_Surface;
struct SDL_Texture;
struct _cairo_surface;
struct _cairo;
struct oshu_display;

/**
 * \defgroup graphics_texture Texture
 * \ingroup graphics
 *
 * \{
 */

struct oshu_texture {
	oshu_size size;
	oshu_point origin;
	struct SDL_Texture *texture;
};

void oshu_draw_texture(struct oshu_display *display, oshu_point p, struct oshu_texture *texture);

void oshu_destroy_texture(struct oshu_texture *texture);

struct oshu_painter {
	oshu_size size;
	oshu_point origin;
	struct SDL_Surface *destination;
	struct _cairo_surface *surface;
	struct _cairo *cr;
};

int oshu_start_painting(oshu_size size, oshu_point origin, struct oshu_painter *painter);

int oshu_finish_painting(struct oshu_painter *painter, struct oshu_display *display, struct oshu_texture *texture);

/** \} */

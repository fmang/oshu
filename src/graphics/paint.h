/**
 * \file graphics/paint.h
 * \ingroup graphics_paint
 */

#pragma once

#include "beatmap/geometry.h"

#include <cairo/cairo.h>

struct SDL_Surface;
struct oshu_display;
struct oshu_texture;

/**
 * \defgroup graphics_paint Paint
 * \ingroup graphics
 *
 * \brief
 * Draw dynamic graphics with Cairo.
 *
 * ```c
 * struct oshu_texture t;
 * struct oshu_painter p;
 * oshu_start_painting(128 + 128 * I, 64 + 64 * I, &p);
 * // call cairo with p->cr
 * oshu_finish_painting(&p, display, &t);
 * ```
 *
 * \{
 */

struct oshu_painter {
	struct oshu_display *display;
	oshu_size size;
	struct SDL_Surface *destination;
	cairo_surface_t *surface;
	cairo_t *cr;
};

/**
 * Create an SDL surface and bind a Cairo context.
 *
 * When you're done drawing, call #oshu_finish_painting.
 *
 * The actual size of the texture depends on the display's current zoom, but
 * this is made transparent with a call to `cairo_scale`. This is what vector
 * graphics are for, right?
 */
int oshu_start_painting(struct oshu_display *display, oshu_size size, struct oshu_painter *painter);

/**
 * Load the drawn texture onto the GPU as a texture, and free everything else.
 *
 * The *painter* object is left in an undefined state, but you may reuse it
 * after #oshu_start_painting. Note that you won't get any performance
 * improvement by reusing painters per se.
 *
 * You should destroy the texture later with #oshu_destroy_texture.
 */
int oshu_finish_painting(struct oshu_painter *painter, struct oshu_texture *texture);

/** \} */

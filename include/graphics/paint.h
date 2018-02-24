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
 * This module provides an SDL texture allocator and Cairo context manager for
 * easy texture generation with Cairo.
 *
 * Unlike a naive Cairo/SDL integration, this module provides full alpha
 * support by un-premultiplying the alpha channel when generating the texture.
 *
 * You can create a cairo context with #oshu_start_painting, and finalize it to
 * upload the texture with #oshu_finish_painting.
 *
 * ```c
 * struct oshu_texture t;
 * struct oshu_painter p;
 * oshu_start_painting(128 + 128 * I, 64 + 64 * I, &p);
 * // call cairo with p->cr
 * oshu_finish_painting(&p, display, &t);
 * ```
 *
 * The \ref graphics/paint.h header imports cairo.h for convenience.
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
 * The *size* argument defines the logical size for #oshu_texture::size, which
 * is the one used when drawing the finalized texture.
 *
 * The actual physical size of the texture depends on the display's current
 * zoom, but this is made transparent with a call to `cairo_scale`, so you can
 * safely ignore that aspect, and assume the size of the cairo surface is the
 * one you mentionned in *size*.
 *
 * Note that the texture quality will depend on the display's current view when
 * this function is called. You should make sure the view set when calling
 * #oshu_start_painting is the same view that will be used with
 * #oshu_draw_texture.
 *
 * The painter object needs not be initialized before calling this function.
 */
int oshu_start_painting(struct oshu_display *display, oshu_size size, struct oshu_painter *painter);

/**
 * Load the drawn texture onto the GPU as a texture, and free everything else.
 *
 * The *painter* object is left in an undefined state, but you may reuse it
 * with #oshu_start_painting. Note that you won't get any performance
 * improvement by reusing painters per se.
 *
 * You must not finish painting on the same painter twice.
 *
 * You must destroy the texture later with #oshu_destroy_texture.
 */
int oshu_finish_painting(struct oshu_painter *painter, struct oshu_texture *texture);

/** \} */

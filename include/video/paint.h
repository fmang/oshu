/**
 * \file video/paint.h
 * \ingroup video_paint
 */

#pragma once

#include "core/geometry.h"

#include <cairo/cairo.h>

struct SDL_Surface;

namespace oshu {

struct display;
struct texture;

/**
 * \defgroup video_paint Paint
 * \ingroup video
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
 * You can create a cairo context with #oshu::start_painting, and finalize it to
 * upload the texture with #oshu::finish_painting.
 *
 * ```c
 * oshu::texture t;
 * oshu::painter p;
 * oshu::start_painting(128 + 128 * I, 64 + 64 * I, &p);
 * // call cairo with p->cr
 * oshu::finish_painting(&p, display, &t);
 * ```
 *
 * The \ref video/paint.h header imports cairo.h for convenience.
 *
 * \{
 */

struct painter {
	oshu::display *display = nullptr;
	oshu::size size = 0;
	struct SDL_Surface *destination = nullptr;
	cairo_surface_t *surface = nullptr;
	cairo_t *cr = nullptr;
};

/**
 * Create an SDL surface and bind a Cairo context.
 *
 * When you're done drawing, call #oshu::finish_painting.
 *
 * The *size* argument defines the logical size for #oshu::texture::size, which
 * is the one used when drawing the finalized texture.
 *
 * The actual physical size of the texture depends on the display's current
 * zoom, but this is made transparent with a call to `cairo_scale`, so you can
 * safely ignore that aspect, and assume the size of the cairo surface is the
 * one you mentionned in *size*.
 *
 * Note that the texture quality will depend on the display's current view when
 * this function is called. You should make sure the view set when calling
 * #oshu::start_painting is the same view that will be used with
 * #oshu::draw_texture.
 *
 * The painter object needs not be initialized before calling this function.
 */
int start_painting(oshu::display *display, oshu::size size, oshu::painter *painter);

/**
 * Load the drawn texture onto the GPU as a texture, and free everything else.
 *
 * The *painter* object is left in an undefined state, but you may reuse it
 * with #oshu::start_painting. Note that you won't get any performance
 * improvement by reusing painters per se.
 *
 * You must not finish painting on the same painter twice.
 *
 * You must destroy the texture later with #oshu::destroy_texture.
 */
int finish_painting(oshu::painter *painter, oshu::texture *texture);

/** \} */

}

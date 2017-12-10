/**
 * \file graphics/view.h
 * \ingroup graphics_view
 */

#pragma once

#include "beatmap/geometry.h"

struct oshu_display;

/**
 * \defgroup graphics_view View
 * \ingroup graphics
 *
 * \{
 */

/**
 * Define a coordinate system.
 *
 * More concretely, this is an affine transformation system, where zoom is the
 * factor and (x, y) the constant. `v(p) = z p + o`
 *
 * Transformation operations on the view like #oshu_resize_view,
 * #oshu_scale_view and #oshu_fit_view are composed on the right:
 * `v(v'(p)) = z (z' p + o') + o = z z' p + z o' + o`
 *
 * For ease of understanding, transformation of views are explained in terms of
 * logical coordinates and physical coordinates. Views convert logical
 * coordinates into physical coordinates, so the logical one is the input, and
 * physical one the output.
 */
struct oshu_view {
	double zoom;
	oshu_point origin;
	oshu_size size;
};

/**
 * Change the size of the view without zooming.
 *
 * The new view's center is aligned with the old one by translating the origin
 * accordingly.
 *
 * This lets you cut the view to introduce margins. Say, you're original view
 * is 400x300, and you resize it to 300x200, then you'll have 50px margins on
 * every side.
 *
 * The aspect ratio needs no be preserved.
 *
 * Definition:
 * - `v(x) = x + (physical width - logical width) / 2`
 *
 * Properties:
 * - `v(logical width / 2) = physical width / 2`
 *
 */
void oshu_resize_view(struct oshu_view *view, oshu_size size);

/**
 * Scale the coordinate system.
 *
 * Scaling by 2 means that a length of 10px will be displayed as 20px.
 *
 * The logical view is downscaled so that it still reflects the physical
 * screen. Scaling a 800x600 view by 2 means that you'll only have 400x300
 * pixels left to draw on.
 *
 * Definition:
 * - `v(x) = factor * x`
 * - `logical width = physical width / factor`
 *
 * Properties:
 * - `v(0) = 0`
 * - `v(logical width) = physical width`
 *
 */
void oshu_scale_view(struct oshu_view *view, double factor);

/**
 * Scale and resize the view while preserving the aspect ratio.
 *
 * The resulting view's logical size is *w × h*, and appears centered and as
 * big as possible in the window, without cutting it.
 *
 * Properties:
 * - The center of the view is the center of the window:
 *   `v(logical width / 2) = physical width / 2`
 * - The view is not cut:
 *   `0 ≤ v(0) ≤ v(logical width) ≤ physical width`
 */
void oshu_fit_view(struct oshu_view *view, oshu_size size);

/**
 * Resize the game area to fit the new size of the window.
 *
 * The size of the window is automatically retrieved from the SDL window.
 *
 * So far, this is a fast operation, but in the future it might require pausing
 * the game to regenerate textures.
 *
 * Call this function when you receive a `SDL_WINDOWEVENT_SIZE_CHANGED`.
 */
void oshu_reset_view(struct oshu_display *display);

/**
 * Project a point from logical coordinates to physical coordinates.
 *
 * Applies the affine transformation: `v(p) = z p + o`.
 *
 * \sa oshu_unproject
 */
oshu_point oshu_project(struct oshu_display *display, oshu_point p);

/**
 * Unproject a point from physical coordinates to logical coordinates.
 *
 * This is the opposite operation of #oshu_project.
 *
 * From the definition of the view, `v(p) = z p + o`,
 * we deduce `p = (v(p) - o) / z`.
 */
oshu_point oshu_unproject(struct oshu_display *display, oshu_point p);

/** \} */

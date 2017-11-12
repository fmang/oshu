/**
 * \file graphics/display.h
 * \ingroup display
 */

#pragma once

#include <SDL2/SDL.h>

#include "beatmap/beatmap.h"

/**
 * \defgroup graphics Graphics
 */

/**
 * \defgroup display Display
 * \ingroup graphics
 *
 * \brief
 * Manage a window and its projections.
 *
 * ### Anatomy of a window
 *
 * SDL provides us with a physical window, whose every pixel is mapped to a
 * distinct pixel on the screen. Its size is arbitrary and the user may resize
 * it at any time. The actual position and size of objects drawn directly on
 * the window is predictable, but you have to take into account the window's
 * size if you don't want to overflow.
 *
 * Inside the window, we have a logicial *viewport*, whose virtual size is
 * always 640x480. When the window is bigger, the viewport is automatically
 * scaled to be as large as possible in the window, without losing its aspect
 * ratio. This is what you'd use to draw game components that should move and
 * scale when the window is resized.
 *
 * The *game area* is a 512x384 rectangle centered inside the viewport. Since
 * game coordinates are used throughout the beatmap, this will be your usual
 * reference.
 *
 * Note that different modes may use differents coordinate systems. The
 * following section will focus on the standard osu mode.
 *
 * ### Coordinate systems
 *
 * The coordinate system the beatmaps use is not the same as the coordinate
 * system of the SDL window. Let's take some time to define it.
 *
 * The original osu! viewport is 640x480 pixels, while the playable game zone
 * is 512x384. The game zone is centered in the viewport, leaving margin for
 * the notes at each corner of the game zone.
 *
 * ```
 * ┌───────────────────────────────────────────────────────────┐
 * │ ↖                                          ↑              │
 * │  (0,0) in viewport coordinates             | 48px         │
 * │  or (-64, -48) in game coordinates         ↓              │
 * │    ┌─────────────────────────────────────────────────┐    │
 * │    │ ↖                                               │    │
 * │    │  (0,0) in game coordinates                      │    │
 * │    │                                                 │    │
 * │    │                                                3│    │4
 * │    │                                                8│    │8
 * │    │                                                4│    │0
 * │    │                                                p│    │p
 * │    │                                                x│    │x
 * │    │                                                 │    │
 * │←——→│                                                 │←——→│
 * │64px│                                                 │64px│
 * │    │                      512px                      │    │
 * │    └─────────────────────────────────────────────────┘    │
 * │                                             ↑             │
 * │                                             | 48px        │
 * │                                             ↓             │
 * └───────────────────────────────────────────────────────────┘
 *                             640px
 * ```
 *
 * When the window grows, this whole viewport is scaled, and not just the game
 * area. The viewport will be zoomed to fit the available space, while
 * preserving the aspect ratio and without cropping. This means that if the
 * ratio of the user's window doesn't have the right ratio, black bars are
 * added, like when playing cinemascope movies.
 *
 * This is what happens when the window is not wide enough:
 *
 * ```
 * ┌───────────────────────────────────────────────────────┐
 * │******    ↕ (window height - 480 × zoom) / 2     ******│
 * ┢━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━┪
 * ┃                                                       ┃
 * ┃                                                       ┃
 * ┃↕ 480 × zoom                                           ┃
 * ┃                                                       ┃
 * ┃                     640 × zoom                        ┃
 * ┡━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━┩
 * │******    ↕ (window height - 480 × zoom) / 2     ******│
 * └───────────────────────────────────────────────────────┘
 *                window width = 640 × zoom
 * ```
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
 * Store everything related to the current display.
 *
 * A display is an SDL window, a renderer, possibly some textures, and also a
 * state for the drawing functions.
 *
 * \sa oshu_open_display
 * \sa oshu_close_display
 */
struct oshu_display {
	SDL_Window *window;
	SDL_Renderer *renderer;
	/**
	 * The current view, used to project coordinates when drawing.
	 *
	 * When the window is resized, make sure you reset it with
	 * #oshu_reset_view and recreate your view from it.
	 */
	struct oshu_view view;
};

/**
 * Create a display structure, open the SDL window and create the renderer.
 *
 * *display* must be null-initialized.
 *
 * \sa oshu_close_display
 */
int oshu_open_display(struct oshu_display *display);

/**
 * Free the display structure and everything associated to it.
 *
 * *display* is left in an unspecified state.
 */
void oshu_close_display(struct oshu_display *display);

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

/**
 * Get the mouse position.
 *
 * SDL provides it in window coordinates, which are unprojected into the
 * current coordinate system for the display.
 *
 * \sa oshu_unproject
 */
oshu_point oshu_get_mouse(struct oshu_display *display);

/**
 * Define a texture loaded on the GPU.
 *
 * Load it with #oshu_load_texture or create it using #oshu_finish_painting,
 * then destroy it with #oshu_destroy_texture.
 */
struct oshu_texture {
	/**
	 * The final size of the texture, specified in logical units.
	 *
	 * Initially, it's the actual size of the texture in pixels, but
	 * typically 1 logical pixel is bigger than a phyiscal pixel, so the
	 * texture will be stretched.
	 *
	 * For example, you will create a 128×128 texture, and then force the
	 * size to 64×64 osu!pixels, which could appear as 96×96 pixels on
	 * screen. Better downscale than upscale, right?
	 */
	oshu_size size;
	/**
	 * The origin defines the anchor of the texture, rather than always using the
	 * top left. Imagine you're representing a circle, you'd rather position it by
	 * its center rather than some imaginary top-left. When drawing a texture at
	 * (x, y), it will be drawn such that the *origin* is at (x, y).
	 */
	oshu_point origin;
	/**
	 * The underlying SDL texture.
	 */
	struct SDL_Texture *texture;
};

/**
 * Load a texture using SDL2_image.
 *
 * Log an error and return -1 on failure.
 */
int oshu_load_texture(struct oshu_display *display, const char *filename, struct oshu_texture *texture);

/**
 * Destroy the SDL texture with `SDL_DestroyTexture`.
 *
 * Note that textures are linked to the renderer they were created for, so make
 * sure you delete the textures first.
 */
void oshu_destroy_texture(struct oshu_texture *texture);

/** \} */

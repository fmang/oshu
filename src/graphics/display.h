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
 * To convert a point from game coordinates to window coordinates, you must,
 * in that order:
 *
 * 1. Translate the point by (64, 48) to obtain viewport coordinates.
 * 2. Multiply by the *zoom* factor for relative window coordinates.
 * 3. Translate by (0, (window height - 480 × zoom) / 2) if the window is too
 *    high, or by ((window width - 640 × zoom) / 2, 0) if the window is too
*     wide.
*
*  Where *zoom* is *window width / 640* if *window width / window height < 640
*  / 480*, or *window height / 480* otherwise.
*
*  The reverse operation, from window coordinates to game coordinates:
*
*  1. Translate by the negative horizontal margin or vertical margin in point 3
*     above.
*  2. Divide both coordinates by the *zoom* factor.
 * 3. Translate the point by (-64, -48).
 *
 * \{
 */

/**
 * See the module's description to understand what these coordinates are.
 */
enum oshu_coordinate_system {
	OSHU_WINDOW_COORDINATES,
	OSHU_VIEWPORT_COORDINATES,
	OSHU_GAME_COORDINATES,
};

/**
 * Position of the viewport in window coordinates, and zoom factor to scale
 * viewport coordinates.
 */
struct oshu_viewport {
	double zoom;
	double x;
	double y;
};

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
	char *window_title;
	SDL_Window *window;
	SDL_Renderer *renderer;
	/**
	 * Current coordinate system.
	 *
	 * Its initial value after #oshu_open_display is unspecified, so you
	 * should set it before drawing anything.
	 *
	 * No function in the #graphics module is going to change it for you,
	 * so you need not check its value between every call to drawing
	 * functions.
	 */
	enum oshu_coordinate_system system;
	/**
	 * Cached information for fast viewport-to-window projections.
	 *
	 * Update it with #oshu_resize_display.
	 */
	struct oshu_viewport viewport;
};

/**
 * Create a display structure, open the SDL window and create the renderer.
 *
 * \sa oshu_close_display
 */
int oshu_open_display(struct oshu_display **display);

/**
 * Free the display structure and everything associated to it.
 */
void oshu_close_display(struct oshu_display **display);

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
void oshu_resize_display(struct oshu_display *display);

/**
 * Get the mouse position.
 *
 * SDL provides it in window coordinates, which are unprojected into the
 * current coordinate system for the display.
 *
 * \sa oshu_unproject
 * \sa oshu_display::system
 */
struct oshu_point oshu_get_mouse(struct oshu_display *display);

/**
 * Project a point on the current coordinate system to SDL window coordinates.
 *
 * \sa oshu_display::system
 */
struct oshu_point oshu_project(struct oshu_display *display, struct oshu_point p);

/**
 * Unproject from SDL window coordinates to the current coordinate system.
 *
 * \sa oshu_display::system
 */
struct oshu_point oshu_unproject(struct oshu_display *display, struct oshu_point p);


/** \} */

/**
 * \file graphics/display.h
 * \ingroup display
 */

#pragma once

#include "beatmap/geometry.h"
#include "graphics/view.h"

struct SDL_Renderer;
struct SDL_Window;

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
 * Store everything related to the current display.
 *
 * A display is an SDL window, a renderer, possibly some textures, and also a
 * state for the drawing functions.
 *
 * \sa oshu_open_display
 * \sa oshu_close_display
 */
struct oshu_display {
	struct SDL_Window *window;
	struct SDL_Renderer *renderer;
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
 * Get the mouse position.
 *
 * SDL provides it in window coordinates, which are unprojected into the
 * current coordinate system for the display.
 *
 * \sa oshu_unproject
 */
oshu_point oshu_get_mouse(struct oshu_display *display);

/** \} */

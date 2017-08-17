/**
 * \file graphics.h
 * \ingroup graphics
 */

#pragma once

#include <SDL2/SDL.h>

#include "beatmap.h"
#include "geometry.h"

/**
 * \defgroup graphics Graphics
 *
 * \brief
 * Manage a window and draw beatmaps.
 *
 * ### Coordinate system
 *
 * The coordinate system the beatmaps use is not the same as the coordinate
 * system of the SDL window. Let's take some time to define it.
 *
 * The original osu! screen is 640x480 pixels, while the playable game zone is
 * 512x384. The game zone is centered in the window, leaving margin for the
 * notes at each corner of the game zone.
 *
 * ```
 * ┌───────────────────────────────────────────────────────────┐
 * │ ↖                                          ↑              │
 * │  (0,0) in window coordinates               | 48px         │
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
 * When the window grows, this whole block is scaled, and not just the scale
 * zone. The window will be zoomed to fit the available space, while preserving
 * the aspect ratio and without cropping. This means that if the ratio of the
 * user's window doesn't have the right ratio, black bars are added, like when
 * playing cinemascope movies.
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
 * To convert a point from game coordinates to physical coordinates, you must,
 * in that order:
 *
 * 1. Translate the point by (64, 48) to obtain original window coordinates.
 * 2. Multiply by the *zoom* factor for zoomed window coordinates.
 * 3. Translate by (0, (window height - 480 × zoom) / 2) if the window is too
 *    high, or by ((window width - 640 × zoom) / 2, 0) if the window is too
*     wide.
*
*  Where *zoom* is *window width / 640* if *window width / window height < 640
*  / 480*, or *window height / 480* otherwise.
*
*  The reverse operation, from physical coordinates to game coordinates:
*
*  1. Translate by the negative horizontal margin or vertical margin in point 3
*     above.
*  2. Divide both coordinates by the *zoom* factor.
 * 3. Translate the point by (-64, -48).
 *
 * \{
 */

/**
 * Store everything related to the current display.
 *
 * Holds the window, and all the textures and sprites required to show the
 * game.
 */
struct oshu_display {
	SDL_Window *window;
	SDL_Renderer *renderer;
	double zoom;
	int horizontal_margin;
	int vertical_margin;
};

/**
 * Create a display structure and load everything required to start playing the
 * game.
 */
int oshu_display_init(struct oshu_display **display);

/**
 * Free the display structure and everything associated to it.
 */
void oshu_display_destroy(struct oshu_display **display);

/**
 * Resize the game area to fit the new size of the window.
 *
 * Call this function when you receive a `SDL_WINDOWEVENT_SIZE_CHANGED`.
 */
void oshu_display_resize(struct oshu_display *display, int w, int h);

/**
 * Get the mouse position in game coordinates.
 */
void oshu_get_mouse(struct oshu_display *display, int *x, int *y);

/**
 * \defgroup draw Draw
 *
 * \brief
 * Rendering functions, from the main beatmap drawing routine to line drawing
 * helpers.
 *
 * \{
 */

/**
 * Draw all the visible nodes from the beatmap, according to the current
 * position in the song.
 *
 * `now` is the current position in the playing song, in seconds.
 */
void oshu_draw_beatmap(struct oshu_display *display, struct oshu_beatmap *beatmap, double now);

/**
 * Draw a hit object.
 */
void oshu_draw_hit(struct oshu_display *display, struct oshu_beatmap *beatmap, struct oshu_hit *hit, double now);

/**
 * Draw a 1-pixel aliased stroke following the path.
 */
void oshu_draw_path(struct oshu_display *display, struct oshu_path *path);

/**
 * Draw a thick stroke following the path.
 *
 * Actually draws two more-or-less parallel lines. Follow the curve nicely but
 * might make ugly loops when the path is too... loopy.
 */
void oshu_draw_thick_path(struct oshu_display *display, struct oshu_path *path, double width);

/**
 * Draw a regular polyline that should look like a circle.
 */
void oshu_draw_circle(struct oshu_display *display, double x, double y, double radius);

/**
 * Draw one line, plain and simple.
 *
 * Perform coordinate translation, unlike its SDL counterpart.
 */
void oshu_draw_line(struct oshu_display *display, int x1, int y1, int x2, int y2);

/** \} */

/** \} */

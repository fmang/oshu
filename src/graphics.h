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
 * \{
 */

/**
 * How big, in pixels, hit objects appear.
 */
extern const int oshu_hit_radius;

/**
 * Store everything related to the current display.
 *
 * Holds the window, and all the textures and sprites required to show the
 * game.
 */
struct oshu_display {
	SDL_Window *window;
	SDL_Renderer *renderer;
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
void oshu_draw_hit(struct oshu_display *display, struct oshu_hit *hit, double now);

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

#pragma once

#include <SDL2/SDL.h>

#include "beatmap.h"
#include "geometry.h"

/**
 * @defgroup graphics Graphics
 *
 * Oshu's display-related routines and structure.
 *
 * @{
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
	SDL_Texture *hit_mark;
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
 * @defgroup draw Draw
 *
 * Rendering functions, from the main \ref oshu_draw_beatmap to line drawing
 * helpers.
 */

/**
 * Draw all the visible nodes from the beatmap, according to the current
 * position in the song.
 *
 * @param msecs Current position in the playing song, in milliseconds.
 */
void oshu_draw_beatmap(struct oshu_display *display, struct oshu_beatmap *beatmap, int msecs);

/**
 * Draw a hit object.
 */
void oshu_draw_hit(struct oshu_display *display, struct oshu_hit *hit);

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

/** @} */

/** @} */

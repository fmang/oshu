#pragma once

#include <SDL2/SDL.h>

#include "beatmap.h"

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
 * Draw all the visible nodes from the beatmap, according to the current
 * position in the song.
 *
 * @param msecs Current position in the playing song, in milliseconds.
 */
void oshu_draw_beatmap(struct oshu_display *display, struct oshu_beatmap *beatmap, int msecs);

/** @} */

/**
 * \file game.h
 * \ingroup game
 *
 * \ref game module definition.
 */

#pragma once

#include "audio.h"
#include "beatmap.h"
#include "graphics.h"

/**
 * \defgroup game Game
 *
 * \brief
 * Coordinate all the modules and implement the game mechanics.
 *
 * \{
 */

/**
 * The full game state, from the beatmap state to the audio and graphical
 * context.
 */
struct oshu_game {
	struct oshu_beatmap *beatmap;
	struct oshu_audio *audio;
	struct oshu_sample *hit_sound;
	struct oshu_display *display;
	/** Will stop a the next iteration if this is true. */
	int stop;
};

/**
 * Create the game context for a beatmap, and load all the associated assets.
 */
int oshu_game_create(const char *beatmap_path, struct oshu_game **game);

/**
 * Free the memory for everything, and set `*game` to *NULL*.
 */
void oshu_game_destroy(struct oshu_game **game);

/**
 * Start the main event loop.
 */
int oshu_game_run(struct oshu_game *game);

/** \} */

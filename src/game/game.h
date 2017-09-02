/**
 * \file game/game.h
 * \ingroup game
 *
 * Define the game module.
 */

#pragma once

#include "audio/audio.h"
#include "beatmap/beatmap.h"
#include "graphics/graphics.h"

#include "modes.h"

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
	struct oshu_game_mode *mode;
	/** Will stop a the next iteration if this is true. */
	int stop;
	/** On autoplay mode, the user interactions are ignored and every
	 *  object will be perfectly hit. */
	int autoplay;
	int paused;
	/** Hit object the user is holding, like a slider. */
	struct oshu_hit *current_hit;
	/** Song position at the previous game loop iteration. */
	double previous_time;
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

/**
 * \file game/game.h
 * \ingroup game
 *
 * Define the game module.
 */

#pragma once

#include "audio/audio.h"
#include "beatmap/beatmap.h"
#include "game/modes.h"
#include "graphics/display.h"

/**
 * \defgroup game Game
 *
 * \brief
 * Coordinate all the modules and implement the game mechanics.
 *
 * \{
 */

/**
 * Keep track of various aspects of the elapsing time.
 */
struct oshu_clock {
	/**
	 * The current time in seconds.
	 *
	 * This is the main clock of the game. Use it unless you have a reason
	 * not to.
	 *
	 * 0 is the beginning of the song. It is totally okay for this clock to
	 * be negative when the beatmap has a lead in time.
	 */
	double now;
	/**
	 * Previous time in seconds.
	 *
	 * This is the time at the previous game loop iteration. It is
	 * occasionaly useful to detect when a specific point in time has just
	 * passed.
	 */
	double before;
	/**
	 * The audio clock.
	 *
	 * It may also be accessed directly using the audio module.
	 *
	 * When the audio hasn't started, it sticks at 0.
	 */
	double audio;
	/**
	 * The process time in ticks.
	 *
	 * This is what `SDL_GetTicks` returns, and is the reference time when
	 * the audio hasn't started.
	 *
	 * It may also be used to increase the accuracy of the game clock,
	 * because sometimes the audio timestamp won't change as no new frame
	 * was decoded.
	 */
	long int ticks;
};

/**
 * The full game state, from the beatmap state to the audio and graphical
 * context.
 */
struct oshu_game {
	struct oshu_beatmap *beatmap;
	struct oshu_audio *audio;
	struct oshu_sample hit_sound;
	struct oshu_display *display;
	struct oshu_game_mode *mode;
	struct oshu_clock clock;
	/** Will stop a the next iteration if this is true. */
	int stop;
	/** On autoplay mode, the user interactions are ignored and every
	 *  object will be perfectly hit. */
	int autoplay;
	int paused;
	/** Background picture. */
	SDL_Texture *background;
	/** Mode-specific data, defined in the \ref game-modes module. */
	union {
		struct oshu_osu_state osu;
	};
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

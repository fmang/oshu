/**
 * \file game/game.h
 * \ingroup game
 *
 * Define the game module.
 */

#pragma once

#include "audio/audio.h"
#include "audio/library.h"
#include "beatmap/beatmap.h"
#include "game/clock.h"
#include "game/mode.h"
#include "ui/ui.h"
#include "graphics/display.h"
#include "osu/osu.h"

struct oshu_game_screen;

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
 *
 * \todo
 * Add a field for the current timing point, and update it from the main loop.
 * It contains the kiai mode and milliseconds per beat, which is in theory
 * needed by most modes.
 */
struct oshu_game {
	struct oshu_beatmap beatmap;
	struct oshu_audio audio;
	struct oshu_sound_library library;
	struct oshu_display display;
	struct oshu_clock clock;
	struct oshu_ui ui;
	/**
	 * The game mode configuration.
	 *
	 * It is always allocated statically, so you must not free it.
	 */
	struct oshu_game_mode *mode;
	int stop;
	int autoplay;
	struct oshu_game_screen *screen;
	/** Mode-specific data, defined inside each mode's header file. */
	union {
		struct osu_state osu;
	};
	/**
	 * Pointer to the next clickable hit.
	 *
	 * Any hit before this cursor has already been dealt with, and its
	 * state should be anything but #OSHU_INITIAL_HIT.
	 *
	 * On the contrary, any hit past this cursor is *usually* in its
	 * initial state, but that's not an absolute necessity. The user might
	 * click a hit far in the future, but still be able to click the one
	 * right before it.
	 *
	 * This cursor is mostly here to speed things up. It is technically
	 * okay for it to always point to the first hit, but the performance
	 * will suffer. It's ideal position is right at the first *fresh*
	 * (= #OSHU_INITIAL_HIT) hit object.
	 *
	 * With the two sentinels in the beatmap's hits linked list, this
	 * cursor is never null, even after the last hit was played.
	 */
	struct oshu_hit *hit_cursor;
};

/**
 * Create the game context for a beatmap, and load all the associated assets.
 *
 * After creating the game, you're free to toy with the options of the game,
 * especially autoplay and pause.
 *
 * When you're done settings the game up, call #oshu_run_game.
 *
 * On failure, the game object is destroyed and left in an unspecified state.
 * On success, it's up to you to destroy it with #oshu_destroy_game.
 */
int oshu_create_game(const char *beatmap_path, struct oshu_game *game);

/**
 * Free all the dynamic memory associated to the game object, but no the object
 * itself.
 */
void oshu_destroy_game(struct oshu_game *game);

/**
 * Start the main event loop.
 *
 * You can stop it with #oshu_stop_game.
 */
int oshu_run_game(struct oshu_game *game);

/**
 * \defgroup game_helpers Helpers
 *
 * \{
 */

/**
 * Find the first hit object after *now - offset*.
 *
 * It bases the search on the #oshu_game::hit_cursor for performance, but its
 * position won't affect the results.
 *
 * For long notes like sliders, the end time is used, not the start time.
 *
 * \sa oshu_look_hit_up
 */
struct oshu_hit* oshu_look_hit_back(struct oshu_game *game, double offset);

/**
 * Find the last hit object before *now + offset*.
 *
 * This is analogous to #oshu_look_hit_back.
 */
struct oshu_hit* oshu_look_hit_up(struct oshu_game *game, double offset);

/**
 * Return the next relevant hit.
 *
 * A hit is irrelevant when it's not supported by the mode, like sliders.
 *
 * The final null hit is considered relevant in order to ensure this function
 * always return something.
 */
struct oshu_hit* oshu_next_hit(struct oshu_game *game);

/**
 * Like #oshu_next_hit, but in the other direction.
 */
struct oshu_hit* oshu_previous_hit(struct oshu_game *game);

/** \} */

/**
 * \defgroup game_actions Actions
 *
 * \{
 */

/**
 * Resume the game.
 *
 * If the music was playing, rewind it by 1 second to leave the player a little
 * break after resuming. This probably makes cheating possible but I couldn't
 * care less.
 *
 * Pausing on a slider will break it though.
 */
void oshu_unpause_game(struct oshu_game *game);

void oshu_pause_game(struct oshu_game *game);

/**
 * Rewind the song by the specified offset in seconds.
 *
 * Rewind the beatmap too but leaving a 1-second break so that we won't seek
 * right before a note.
 */
void oshu_rewind_game(struct oshu_game *game, double offset);

/**
 * See #oshu_rewind_game.
 */
void oshu_forward_game(struct oshu_game *game, double offset);

/**
 * Make the game stop at the next iteration.
 *
 * It can be called from a signal handler.
 */
void oshu_stop_game(struct oshu_game *game);

/** \} */

/** \} */

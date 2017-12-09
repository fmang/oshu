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
#include "game/mode.h"
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
	 * The process time, computed from SDL's ticks.
	 *
	 * This is the reference time when the audio hasn't started, or when it
	 * has stopped.
	 *
	 * It may also be used to increase the accuracy of the game clock,
	 * because sometimes the audio timestamp won't change as no new frame
	 * was decoded.
	 */
	double system;
};

/**
 * Enumerate the game states.
 *
 * These are flags and can be OR'd.
 */
enum oshu_game_state {
	/**
	 * The user plays.
	 *
	 * It can be combined with OSHU_PLAYING or OSHU_PAUSED, but not paused.
	 */
	OSHU_USERPLAY = 0x01,
	/**
	 * Tells whether the game is in autoplay mode or not.
	 *
	 * Paused autoplay and playing autoplay are both possible.
	 *
	 * It cannot be combined with #OSHU_USERPLAY.
	 */
	OSHU_AUTOPLAY = 0x02,
	/**
	 * The song is playing, whether it is on autoplay or not.
	 */
	OSHU_PLAYING = 0x04,
	/**
	 * The game is paused.
	 *
	 * This is contradictory to OSHU_PLAYING.
	 */
	OSHU_PAUSED = 0x08,
	/**
	 * After the final note has been playing, the game enters some
	 * pseudo-pause mode to display the score.
	 */
	OSHU_FINISHED = 0x10,
	/**
	 * Will stop a the next iteration if this is true.
	 */
	OSHU_STOPPING = 0x20,
};

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
	/**
	 * The game mode configuration.
	 *
	 * It is always allocated statically, so you must not free it.
	 */
	struct oshu_game_mode *mode;
	/** Combination of flags from #oshu_game_state. */
	int state;
	int autoplay;
	struct oshu_game_screen *screen;
	/** Background picture. */
	struct oshu_texture background;
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
 */
int oshu_run_game(struct oshu_game *game);

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
 * Show the state of the game (paused/playing) and the current song position.
 *
 * Only do that for terminal outputs in order not to spam something if the
 * output is redirected.
 *
 * The state length must not decrease over time, otherwise you end up with
 * glitches. If you write `foo\rx`, you get `xoo`. This is the reason the
 * Paused string literal has an extra space.
 */
void oshu_print_state(struct oshu_game *game);

/** \} */

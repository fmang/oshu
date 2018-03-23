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
#include "game/controls.h"
#include "video/display.h"

/**
 * \defgroup game Game
 *
 * \brief
 * Coordinate all the modules.
 *
 * \{
 */

/**
 * The full game state, from the beatmap state to the audio and graphical
 * context.
 */
struct oshu_game_state {
	/**
	 * \todo
	 * Take the beatmap by reference when the game state is constructed.
	 */
	struct oshu_beatmap beatmap;
	struct oshu_audio audio;
	struct oshu_sound_library library;
	struct oshu_display display;
	struct oshu_clock clock;
	int stop;
	int autoplay;
	bool paused;
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
 * The game base object.
 *
 * It is defined as a game state with a set of pure virtual methods to
 * implement.
 *
 * Game modes inherit from this class and implement all these methods.
 *
 * \todo
 * This should be turned into a oshu::game::mode interface, and not inherit a
 * state. If anything, the state should inherit the mode and called *base_game*
 * or something like that.
 */
struct oshu_game : public oshu_game_state {

	virtual ~oshu_game() = default;

	/**
	 * Initialize the mode-specific objects.
	 *
	 * \todo
	 * Port to RAII.
	 */
	virtual int initialize() = 0;

	/**
	 * Initialize the mode-specific objects.
	 *
	 * \todo
	 * Port to RAII.
	 */
	virtual int destroy() = 0;

	/**
	 * Called at every game iteration, unless the game is paused.
	 *
	 * The job of this function is to check the game clock and see if notes
	 * were missed, or other things of the same kind.
	 *
	 * There's no guarantee this callback is called at regular intervals.
	 *
	 * For autoplay, use #autoplay instead.
	 */
	virtual int check() = 0;

	/**
	 * Called pretty much like #check, except it's for autoplay mode.
	 */
	virtual int check_autoplay() = 0;

	/**
	 * Handle a key press keyboard event, or mouse button press event.
	 *
	 * Key repeats are filtered out by the parent module, along with any
	 * key used by the game module itself, like escape or space to pause, q
	 * to quit, &c. Same goes for mouse buttons.
	 *
	 * If you need the mouse position, use #oshu_get_mouse to have it in
	 * game coordinates.
	 *
	 * This callback isn't called when the game is paused or on autoplay.
	 *
	 * \sa release
	 */
	virtual int press(enum oshu_finger key) = 0;

	/**
	 * See #press.
	 */
	virtual int release(enum oshu_finger key) = 0;

	/**
	 * Release any held object, like sliders or hold notes.
	 *
	 * This function is called whenever the user seeks somewhere in the
	 * song.
	 */
	virtual int relinquish() = 0;

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
 *
 * \todo
 * It should not be the responsibility of this module to load the beatmap. If
 * the beatmap is a taiko beatmap, then the taiko game should be instanciated,
 * not the base module. Instead, take a beatmap by reference.
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
 * \brief
 * Read-only game accessors.
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
 * \brief
 * Change the game state.
 *
 * \{
 */

/**
 * Resume the game.
 *
 * \sa oshu_pause_game
 */
void oshu_unpause_game(struct oshu_game *game);

/**
 * Pause the game.
 *
 * \sa oshu_unpause_game
 */
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

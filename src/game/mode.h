/**
 * \file game/mode.h
 * \ingroup game
 *
 * \brief
 * Generic interface to all game modes.
 */

#pragma once

#include "game/controls.h"

/**
 * \ingroup game
 * \{
 */

struct oshu_game;

/**
 * A generic mode is defined by a structure of callbacks.
 *
 * All these callbacks take the game state, and return 0 on success, -1 on
 * error. An error is likely to stop the game in the future, so make sure it's
 * worth it.
 *
 * All the callbacks must be defined! It's okay if it doesn't do anything, but
 * the pointers must point somewhere.
 *
 * \todo
 * Add an initialization method, and a destroy method.
 */
struct oshu_game_mode {
	/**
	 * Initialize all the required dynamic resources.
	 *
	 * \sa destroy
	 */
	int (*initialize)(struct oshu_game *game);
	/**
	 * Destroy any allocated dynamic memory.
	 */
	int (*destroy)(struct oshu_game *game);
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
	int (*check)(struct oshu_game *game);
	/**
	 * Called pretty much like #check, except it's for autoplay mode.
	 */
	int (*autoplay)(struct oshu_game *game);
	/**
	 * Draw the game on the display.
	 */
	int (*draw)(struct oshu_game *game);
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
	int (*press)(struct oshu_game *game, enum oshu_finger key);
	/**
	 * See #press.
	 */
	int (*release)(struct oshu_game *game, enum oshu_finger key);
	/**
	 * Release any held object, like sliders or hold notes.
	 *
	 * This function is called whenever the user seeks somewhere in the
	 * song.
	 */
	int (*relinquish)(struct oshu_game *game);
};

/** \} */

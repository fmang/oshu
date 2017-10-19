/**
 * \file game/mode.h
 * \ingroup game
 *
 * \brief
 * Generic interface to all game modes.
 */

#pragma once

#include <SDL2/SDL.h>

/**
 * \ingroup game
 * \{
 */

struct oshu_game;

/**
 * The various keys and buttons.
 *
 * This makes the intention clearer, as we're identifying fingers rather than
 * letters. The translation is made in the common game module.
 *
 * See https://osu.ppy.sh/help/wiki/shared/Mania_key_layouts.jpg
 * and more specially https://osu.ppy.sh/help/wiki/shared/Mania_key_layouts.jpg
 *
 * The alternative keys, on the right side of the picture, won't be supported
 * until I figure out how you're supposed to play with them.
 *
 * The osu! keys are Z and X, mapped to the left middle finger and left index,
 * respectively. Note that it's one-handed because the right hand is busy with
 * the mouse.
 *
 * The taiko keys are Z, X, C, V, mapped to left middle finger, index, right
 * index, middle finger, respectively.
 *
 * Because the keys' positions are crucial, the keyboard keys should be
 * detected in a layout-agnostic way, from the physical codes.
 */
enum oshu_key {
	OSHU_LEFT_PINKY = -4,
	OSHU_LEFT_RING = -3,
	OSHU_LEFT_MIDDLE = -2,
	OSHU_LEFT_INDEX = -1,
	OSHU_THUMBS = 0,
	OSHU_RIGHT_INDEX = 1,
	OSHU_RIGHT_MIDDLE = 2,
	OSHU_RIGHT_RING = 3,
	OSHU_RIGHT_PINKY = 4,
	/**
	 * Playing with the mouse is discouraged, but if you really wanna do
	 * this, the buttons are mapped to values overlapping those of keys,
	 * because the mouse will always be a substitute rather than a real
	 * thing.
	 *
	 * The game module, of course, will have access to the original event
	 * and may do whatever it pleases with it, to navigate menus for
	 * example. The game modes, however, have no business knowing that.
	 */
	OSHU_LEFT_BUTTON = -1,
	OSHU_MIDDLE_BUTTON = 0,
	OSHU_RIGHT_BUTTON = 1,
};

/**
 * A generic mode is defined by a structure of callbacks.
 *
 * All these callbacks take the game state, and return 0 on success, -1 on
 * error. An error is likely to stop the game in the future, so make sure it's
 * worth it.
 *
 * When a callback is null, it means there's nothing to do, and that's
 * perfectly fine. For example, the taiko or mania modes have no reason to use
 * the mouse.
 */
struct oshu_game_mode {
	/**
	 * Called at every game iteration, unless the game is paused.
	 *
	 * The job of this function is to check the game clock and see if notes
	 * were missed, or other things of the same kind.
	 *
	 * There's no guarantee this callback is called at regular intervals.
	 */
	int (*check)(struct oshu_game *game);
	/**
	 * Draw the game on the display.
	 */
	int (*draw)(struct oshu_game *game);
	/**
	 * Handle a key press keyboard event.
	 *
	 * Key repeats are filtered out by the parent module, along with any
	 * key used by the game module itself, like escape or space to pause, q
	 * to quit, &c.
	 *
	 * This callback isn't called when the game is paused or on autoplay.
	 *
	 * \sa key_released
	 */
	int (*key_pressed)(struct oshu_game *game, SDL_Keysym *key);
	/**
	 * See #key_released.
	 */
	int (*key_released)(struct oshu_game *game, SDL_Keysym *key);
	/**
	 * Handle a mouse button press event.
	 *
	 * Mouse click events may be filtered by the parent game module in the
	 * future to implement mode-agnotistic features, but you should not
	 * worry about it.
	 *
	 * If you need the mouse position, use #oshu_get_mouse to have it in
	 * game coordinates.
	 *
	 * This callback isn't called when the game is paused or on autoplay.
	 *
	 * \sa mouse_released.
	 */
	int (*mouse_pressed)(struct oshu_game *game, Uint8 button);
	/**
	 * See #mouse_released.
	 */
	int (*mouse_released)(struct oshu_game *game, Uint8 button);
};

/** \} */

/**
 * \file game/mode.h
 * \ingroup game
 *
 * \brief
 * Generic interface to all game modes.
 */

#pragma once

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
 *
 * Keys on the left hand are negative, while keys on the right hand are
 * positive, making it easy to test which hand a constant belongs to.
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
	 * Playing with the mouse is discouraged, but if you really wanna be
	 * able to catch the clicks, this is for you. In particular, it is nice
	 * for newcomers, as clicking is more intuitive than pressing X.
	 *
	 * The game module has have access to the original event and may do
	 * whatever it pleases with it, to navigate menus for example. The game
	 * modes, however, have no business knowing that, and should handle
	 * clicks like keys.
	 */
	OSHU_LEFT_BUTTON = -5,
	/**
	 * See #OSHU_LEFT_BUTTON.
	 */
	OSHU_RIGHT_BUTTON = 5,
	/**
	 * Special value when a key couldn't be translated.
	 *
	 * 0 and -1 are already taken, so it's some arbitrary negative value,
	 * which makes it look like it's for the left hand.
	 *
	 * Anyway, this key is only meant as a temporary value, and it is never
	 * passed to the #oshu_game_mode::pressed. No need to make special
	 * cases for it. However, because new keys might be added in the future
	 * (who knows?) you should have a sane default case for keys your
	 * module doesn't handle, if relevant.
	 */
        OSHU_UNKNOWN_KEY = -100,
};

/**
 * A generic mode is defined by a structure of callbacks.
 *
 * All these callbacks take the game state, and return 0 on success, -1 on
 * error. An error is likely to stop the game in the future, so make sure it's
 * worth it.
 *
 * All the callbacks must be defined! It's okay if it doesn't do anything, but
 * the pointers must point somewhere.
 */
struct oshu_game_mode {
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
	 * \sa released
	 */
	int (*pressed)(struct oshu_game *game, enum oshu_key key);
	/**
	 * See #pressed.
	 */
	int (*released)(struct oshu_game *game, enum oshu_key key);
};

/** \} */

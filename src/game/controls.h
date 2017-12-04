/**
 * \file game/controls.h
 * \ingroup game_controls
 */

#pragma once

#include <SDL2/SDL_keycode.h>

struct SDL_Keysym;

/**
 * \defgroup game_controls Controls
 * \ingroup game
 *
 * \brief
 * Define the keyboard controls.
 *
 * \{
 */

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
	 * passed to the #oshu_game_mode::press. No need to make special
	 * cases for it. However, because new keys might be added in the future
	 * (who knows?) you should have a sane default case for keys your
	 * module doesn't handle, if relevant.
	 */
        OSHU_UNKNOWN_KEY = -100,
};

/**
 * Translate an SDL keysym to its finger in #oshu_key.
 *
 * The physical key code is used for the translation, to guarantee layout agnosticity.
 *
 * If the key doesn't match a finger, return #OSHU_UNKNOWN_KEY.
 */
enum oshu_key oshu_translate_key(struct SDL_Keysym *keysym);

/**
 * Map functions to SDL keycodes.
 *
 * This is meant to ensure consistency across the various game screens. With
 * this, no two screens would use different keys for the same function.
 *
 * Compare the keys with `SDL_Event.key.keysym.sym`.
 */
enum oshu_control_key {
	OSHU_QUIT_KEY = SDLK_q,
	OSHU_PAUSE_KEY = SDLK_ESCAPE,
	OSHU_REWIND_KEY = SDLK_PAGEUP,
	OSHU_FORWARD_KEY = SDLK_PAGEDOWN,
};

/** \} */

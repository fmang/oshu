/**
 * \file game/controls.cc
 * \ingroup game_controls
 */

#include "game/controls.h"

#include <SDL2/SDL.h>

enum oshu::finger oshu::translate_key(SDL_Keysym *keysym)
{
	switch (keysym->scancode) {
	/* Bottom row, for standard and taiko modes. */
	case SDL_SCANCODE_Z: return oshu::LEFT_MIDDLE;
	case SDL_SCANCODE_X: return oshu::LEFT_INDEX;
	case SDL_SCANCODE_C: return oshu::RIGHT_INDEX;
	case SDL_SCANCODE_V: return oshu::RIGHT_MIDDLE;
	/* Middle row, for mania. */
	case SDL_SCANCODE_A: return oshu::LEFT_PINKY;
	case SDL_SCANCODE_S: return oshu::LEFT_RING;
	case SDL_SCANCODE_D: return oshu::LEFT_MIDDLE;
	case SDL_SCANCODE_F: return oshu::LEFT_INDEX;
	case SDL_SCANCODE_SPACE: return oshu::THUMBS;
	case SDL_SCANCODE_J: return oshu::RIGHT_INDEX;
	case SDL_SCANCODE_K: return oshu::RIGHT_MIDDLE;
	case SDL_SCANCODE_L: return oshu::RIGHT_RING;
	case SDL_SCANCODE_SEMICOLON: return oshu::RIGHT_PINKY;
	default:             return oshu::UNKNOWN_KEY;
	}
}

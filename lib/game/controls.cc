/**
 * \file game/controls.cc
 * \ingroup game_controls
 */

#include "game/controls.h"

#include <SDL2/SDL.h>

enum oshu_finger oshu_translate_key(SDL_Keysym *keysym)
{
	switch (keysym->scancode) {
	/* Bottom row, for standard and taiko modes. */
	case SDL_SCANCODE_Z: return OSHU_LEFT_MIDDLE;
	case SDL_SCANCODE_X: return OSHU_LEFT_INDEX;
	case SDL_SCANCODE_C: return OSHU_RIGHT_INDEX;
	case SDL_SCANCODE_V: return OSHU_RIGHT_MIDDLE;
	/* Middle row, for mania. */
	case SDL_SCANCODE_A: return OSHU_LEFT_PINKY;
	case SDL_SCANCODE_S: return OSHU_LEFT_RING;
	case SDL_SCANCODE_D: return OSHU_LEFT_MIDDLE;
	case SDL_SCANCODE_F: return OSHU_LEFT_INDEX;
	case SDL_SCANCODE_SPACE: return OSHU_THUMBS;
	case SDL_SCANCODE_J: return OSHU_RIGHT_INDEX;
	case SDL_SCANCODE_K: return OSHU_RIGHT_MIDDLE;
	case SDL_SCANCODE_L: return OSHU_RIGHT_RING;
	case SDL_SCANCODE_SEMICOLON: return OSHU_RIGHT_PINKY;
	default:             return OSHU_UNKNOWN_KEY;
	}
}

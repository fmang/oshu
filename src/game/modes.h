/**
 * \file game/modes.h
 * \ingroup game-modes
 *
 * Define the game module.
 */

#pragma once

#include <SDL2/SDL.h>

/**
 * \defgroup game-modes Modes
 * \ingroup game
 *
 * \brief
 * Generic interface to all game modes.
 *
 * \{
 */

struct oshu_game;

struct oshu_game_mode {
	int (*check)(struct oshu_game *game);
	int (*draw)(struct oshu_game *game);
	int (*key_pressed)(struct oshu_game *game, SDL_Keysym *key);
	int (*key_released)(struct oshu_game *game, SDL_Keysym *key);
	int (*mouse_pressed)(struct oshu_game *game, Uint8 button);
	int (*mouse_released)(struct oshu_game *game, Uint8 button);
};

struct oshu_osu_state {
	/** Hit object the user is holding, like a slider. */
	struct oshu_hit *current_slider;
};

extern struct oshu_game_mode oshu_osu_mode;

/** \} */

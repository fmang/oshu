/**
 * \file game/osu/osu.h
 * \ingroup osu
 */

#pragma once

#include "game/mode.h"

struct oshu_hit;

/**
 * \defgroup osu Osu
 * \ingroup game
 *
 * \brief
 * Osu standard game mode.
 *
 * \{
 */

/**
 * Parts of the game state specific to the Osu! standard mode.
 */
struct oshu_osu_state {
	/**
	 * Slider hit object the user is holding.
	 *
	 * NULL most of the time.
	 */
	struct oshu_hit *current_slider;
};

/**
 * Implementation of the standard osu! game mode.
 */
extern struct oshu_game_mode oshu_osu_mode;

/** \} */

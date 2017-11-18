/**
 * \file game/osu/osu.h
 * \ingroup osu
 */

#pragma once

#include "game/mode.h"
#include "graphics/display.h"

struct oshu_beatmap;
struct oshu_display;
struct oshu_game_mode;
struct oshu_hit;

struct SDL_Texture;

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
 * Parts of the game state specific to osu!standard mode.
 */
struct osu_state {
	/**
	 * Slider hit object the user is holding.
	 *
	 * NULL most of the time.
	 */
	struct oshu_hit *current_slider;
	/**
	 * Keyboard key or mouse button associated to the #current_slider.
	 *
	 * When the #current_slider is NULL, the value of this field is
	 * irrelevant.
	 */
	enum oshu_key held_key;
	/**
	 * Dynamic array of circle hit object textures.
	 *
	 * There are as many textures as there are colors in the beatmap.
	 */
	struct oshu_texture *circles;
	/**
	 * Full-size approach circle.
	 *
	 * Its size is the `radius + approach_size` from the beatmap.
	 */
	struct oshu_texture approach_circle;
	/**
	 * The slider ball and its tolerance circle.
	 */
	struct oshu_texture slider_ball;
	/**
	 * Symbol to indicate a note was successfully hit.
	 *
	 * A green circle.
	 */
	struct oshu_texture good_mark;
	/**
	 * Symbol to indicate a note was missed.
	 *
	 * A red X.
	 */
	struct oshu_texture bad_mark;
	/**
	 * Symbol to indicate a note was skipped.
	 *
	 * A blue triangle pointing right.
	 */
	struct oshu_texture skip_mark;
};

/**
 * Paint all the required textures for the beatmap.
 *
 * Free everything with #osu_free_resources.
 *
 * \todo
 * Paint textures lazily, or in a parallel thread for a faster start-up.
 */
int osu_paint_resources(struct oshu_game *game);

int osu_paint_slider(struct oshu_game *game, struct oshu_hit *hit);

/**
 * Free the dynamic resources of the game mode.
 */
int osu_free_resources(struct oshu_game *game);

/**
 * The main drawing callback of the mode.
 *
 * Exported in its own sub-module.
 */
int osu_draw(struct oshu_game *game);

/**
 * Implementation of the standard osu! game mode.
 */
extern struct oshu_game_mode osu_mode;

/** \} */

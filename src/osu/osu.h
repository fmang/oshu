/**
 * \file osu/osu.h
 * \ingroup osu
 */

#pragma once

#include "game/controls.h"
#include "graphics/texture.h"

struct oshu_game;
struct oshu_game_mode;
struct oshu_hit;

/**
 * \defgroup osu Osu
 *
 * \brief
 * osu!standard game mode.
 *
 * Most of the functions defined in this module are internal.
 *
 * It is connected to the game module by its #osu_state structure and #osu_mode
 * object.
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
	enum oshu_finger held_key;
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
	 * Symbol for early hits.
	 *
	 * A yellow half-circle, on the left.
	 */
	struct oshu_texture early_mark;
	/**
	 * Symbol for early hits.
	 *
	 * A yellow half-circle, on the right.
	 */
	struct oshu_texture late_mark;
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
	/**
	 * Little tick mark for the dotted line between two consecutive hits.
	 */
	struct oshu_texture connector;
};

/**
 * Paint all the required textures for the beatmap.
 *
 * Free everything with #osu_free_resources.
 *
 * Sliders are not painted. Instead, you must call #osu_paint_slider.
 */
int osu_paint_resources(struct oshu_game *game);

/**
 * Paint a slider.
 *
 * This is externalized from #osu_paint_slider to let you paint them lazily,
 * because painting all the sliders at once would increase the startup time by
 * up to a few long seconds.
 *
 * The texture is stored in `hit->texture`.
 *
 * Slider textures are freed with #osu_free_resources.
 */
int osu_paint_slider(struct oshu_game *game, struct oshu_hit *hit);

/**
 * Free the dynamic resources of the game mode.
 */
void osu_free_resources(struct oshu_game *game);

/**
 * The main drawing callback of the mode.
 *
 * Exported in its own sub-module because it is relatively complex.
 */
int osu_draw(struct oshu_game *game);

/**
 * Set-up the coordinate system for the osu!standard mode.
 *
 * The window is mapped to a 640×480 screen with a few margins, yielding a
 * 512×384 logical screen.
 *
 * It is the caller's responsibility to reset the view, preferably such that
 * the #osu_view/#oshu_reset_view pairing looks obvious.
 */
void osu_view(struct oshu_game *game);

/**
 * Implementation of the standard osu! game mode.
 */
extern struct oshu_game_mode osu_mode;

/** \} */

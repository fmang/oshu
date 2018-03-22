/**
 * \file osu/osu.h
 * \ingroup osu
 */

#pragma once

#include "game/game.h"
#include "video/texture.h"
#include "gui/cursor.h"

/**
 * \defgroup osu Osu
 *
 * \brief
 * osu!standard game mode.
 *
 * \todo
 * In the future, this should be split into two modules: the game mechanics in
 * the game module, and the renderer in the gui module.
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
	/**
	 * Use a fancy software cursor for the osu!standard mode, because the
	 * mouse is a central part of the gameplay.
	 */
	struct oshu_cursor_widget cursor;
};

struct osu_game : public oshu_game {
	struct osu_state osu;

	int initialize() override;
	int destroy() override;
	int check() override;
	int check_autoplay() override;
	int draw() override;
	int press(enum oshu_finger key) override;
	int release(enum oshu_finger key) override;
	int relinquish() override;
};

/**
 * Paint all the required textures for the beatmap.
 *
 * Free everything with #osu_free_resources.
 *
 * Sliders are not painted. Instead, you must call #osu_paint_slider.
 */
int osu_paint_resources(struct osu_game *game);

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
int osu_paint_slider(struct osu_game *game, struct oshu_hit *hit);

/**
 * Free the dynamic resources of the game mode.
 */
void osu_free_resources(struct osu_game *game);

/**
 * Set-up the coordinate system for the osu!standard mode.
 *
 * The window is mapped to a 640×480 screen with a few margins, yielding a
 * 512×384 logical screen.
 *
 * It is the caller's responsibility to reset the view, preferably such that
 * the #osu_view/#oshu_reset_view pairing looks obvious.
 */
void osu_view(struct osu_game *game);

/** \} */

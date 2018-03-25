/**
 * \file include/ui/osu.h
 * \ingroup ui
 */

#pragma once

#include "game/controls.h"
#include "ui/cursor.h"
#include "ui/widget.h"
#include "video/texture.h"

#include <memory>

struct osu_game;

namespace oshu {
namespace ui {

struct osu;

struct osu_mouse : public oshu::game::mouse {
	osu_mouse(oshu_display *display);
	oshu_display *display;
	oshu_point position() override;
};

/**
 * \ingroup ui
 * \{
 */

struct osu : public widget {
	osu(oshu_display *display, osu_game &game);
	~osu();

	oshu_display *display;
	osu_game &game;
	std::shared_ptr<osu_mouse> mouse;

	void draw() override;

	/**
	 * Dynamic array of circle hit object textures.
	 *
	 * There are as many textures as there are colors in the beatmap.
	 */
	struct oshu_texture *circles {};
	/**
	 * Full-size approach circle.
	 *
	 * Its size is the `radius + approach_size` from the beatmap.
	 */
	struct oshu_texture approach_circle {};
	/**
	 * The slider ball and its tolerance circle.
	 */
	struct oshu_texture slider_ball {};
	/**
	 * Symbol to indicate a note was successfully hit.
	 *
	 * A green circle.
	 */
	struct oshu_texture good_mark {};
	/**
	 * Symbol for early hits.
	 *
	 * A yellow half-circle, on the left.
	 */
	struct oshu_texture early_mark {};
	/**
	 * Symbol for early hits.
	 *
	 * A yellow half-circle, on the right.
	 */
	struct oshu_texture late_mark {};
	/**
	 * Symbol to indicate a note was missed.
	 *
	 * A red X.
	 */
	struct oshu_texture bad_mark {};
	/**
	 * Symbol to indicate a note was skipped.
	 *
	 * A blue triangle pointing right.
	 */
	struct oshu_texture skip_mark {};
	/**
	 * Little tick mark for the dotted line between two consecutive hits.
	 */
	struct oshu_texture connector {};
	/**
	 * Use a fancy software cursor for the osu!standard mode, because the
	 * mouse is a central part of the gameplay.
	 */
	struct oshu_cursor_widget cursor {};
};

/** \} */

}}

/**
 * Paint all the required textures for the beatmap.
 *
 * Free everything with #osu_free_resources.
 *
 * Sliders are not painted. Instead, you must call #osu_paint_slider.
 */
int osu_paint_resources(oshu::ui::osu&);

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
int osu_paint_slider(oshu::ui::osu&, struct oshu_hit *hit);

/**
 * Free the dynamic resources of the game mode.
 */
void osu_free_resources(oshu::ui::osu&);

/**
 * Set-up the coordinate system for the osu!standard mode.
 *
 * The window is mapped to a 640×480 screen with a few margins, yielding a
 * 512×384 logical screen.
 *
 * It is the caller's responsibility to reset the view, preferably such that
 * the #osu_view/#oshu_reset_view pairing looks obvious.
 */
void osu_view(struct oshu_display *display);

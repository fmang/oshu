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

namespace oshu {

struct hit;
struct osu_ui;
struct osu_game;

struct osu_mouse : public oshu::mouse {
	osu_mouse(oshu::display *display);
	oshu::display *display;
	oshu::point position() override;
};

/**
 * \ingroup ui
 * \{
 */

struct osu_ui : public widget {
	osu_ui(oshu::display *display, oshu::osu_game &game);
	~osu_ui();

	oshu::display *display;
	oshu::osu_game &game;
	std::shared_ptr<osu_mouse> mouse;

	void draw() override;

	/**
	 * Dynamic array of circle hit object textures.
	 *
	 * There are as many textures as there are colors in the beatmap.
	 */
	oshu::texture *circles {};
	/**
	 * Full-size approach circle.
	 *
	 * Its size is the `radius + approach_size` from the beatmap.
	 */
	oshu::texture approach_circle {};
	/**
	 * The slider ball and its tolerance circle.
	 */
	oshu::texture slider_ball {};
	/**
	 * Symbol to indicate a note was successfully hit.
	 *
	 * A green circle.
	 */
	oshu::texture good_mark {};
	/**
	 * Symbol for early hits.
	 *
	 * A yellow half-circle, on the left.
	 */
	oshu::texture early_mark {};
	/**
	 * Symbol for early hits.
	 *
	 * A yellow half-circle, on the right.
	 */
	oshu::texture late_mark {};
	/**
	 * Symbol to indicate a note was missed.
	 *
	 * A red X.
	 */
	oshu::texture bad_mark {};
	/**
	 * Symbol to indicate a note was skipped.
	 *
	 * A blue triangle pointing right.
	 */
	oshu::texture skip_mark {};
	/**
	 * Little tick mark for the dotted line between two consecutive hits.
	 */
	oshu::texture connector {};
	/**
	 * Use a fancy software cursor for the osu!standard mode, because the
	 * mouse is a central part of the gameplay.
	 */
	oshu::cursor_widget cursor {};
};

/**
 * Paint all the required textures for the beatmap.
 *
 * Free everything with #oshu::osu_free_resources.
 *
 * Sliders are not painted. Instead, you must call #oshu::osu_paint_slider.
 */
int osu_paint_resources(oshu::osu_ui&);

/**
 * Paint a slider.
 *
 * This is externalized from #oshu::osu_paint_slider to let you paint them lazily,
 * because painting all the sliders at once would increase the startup time by
 * up to a few long seconds.
 *
 * The texture is stored in `hit->texture`.
 *
 * Slider textures are freed with #oshu::osu_free_resources.
 */
int osu_paint_slider(oshu::osu_ui&, oshu::hit *hit);

/**
 * Free the dynamic resources of the game mode.
 */
void osu_free_resources(oshu::osu_ui&);

/**
 * Set-up the coordinate system for the osu!standard mode.
 *
 * The window is mapped to a 640×480 screen with a few margins, yielding a
 * 512×384 logical screen.
 *
 * It is the caller's responsibility to reset the view, preferably such that
 * the #oshu::osu_view/#oshu::reset_view pairing looks obvious.
 */
void osu_view(oshu::display *display);

/** \} */

}

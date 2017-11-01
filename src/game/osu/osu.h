/**
 * \file game/osu/osu.h
 * \ingroup osu
 */

#pragma once

#include "game/mode.h"

struct oshu_beatmap;
struct oshu_display;
struct oshu_game_mode;
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
};

/**
 * Draw a hit object.
 */
void osu_draw_hit(struct oshu_display *display, struct oshu_beatmap *beatmap, struct oshu_hit *hit, double now);

/**
 * Draw all the visible nodes from the beatmap, according to the current
 * position in the song.
 *
 * `now` is the current position in the playing song, in seconds.
 */
void osu_draw_beatmap(struct oshu_display *display, struct oshu_beatmap *beatmap, struct oshu_hit *cursor, double now);

/**
 * Implementation of the standard osu! game mode.
 */
extern struct oshu_game_mode osu_mode;

/** \} */

/**
 * \file include/game/osu.h
 * \ingroup game_osu
 */

#pragma once

#include "game/base.h"

#include <memory>

/**
 * \defgroup game_osu Osu
 * \ingroup game
 *
 * \brief
 * osu!standard game mode.
 *
 * \{
 */

struct osu_game : public oshu::game::base {
	osu_game(const char *beatmap_path);

	/**
	 * Slider hit object the user is holding.
	 *
	 * NULL most of the time.
	 */
	struct oshu_hit *current_slider {};
	/**
	 * Keyboard key or mouse button associated to the #current_slider.
	 *
	 * When the #current_slider is NULL, the value of this field is
	 * irrelevant.
	 */
	enum oshu_finger held_key {};
	std::shared_ptr<oshu::game::mouse> mouse {};

	int check() override;
	int check_autoplay() override;
	int press(enum oshu_finger key) override;
	int release(enum oshu_finger key) override;
	int relinquish() override;
};

/** \} */

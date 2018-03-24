/**
 * \file include/game/osu.h
 * \ingroup game_osu
 */

#pragma once

#include "game/game.h"

#include <memory>

/**
 * \defgroup game_osu Osu
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
	std::shared_ptr<oshu::game::mouse> mouse;
};

struct osu_game : public oshu_game {
	struct osu_state osu;

	int initialize() override;
	int destroy() override;
	int check() override;
	int check_autoplay() override;
	int press(enum oshu_finger key) override;
	int release(enum oshu_finger key) override;
	int relinquish() override;
};

/** \} */

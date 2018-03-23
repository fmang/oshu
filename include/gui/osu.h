/**
 * \file include/gui/osu.h
 * \ingroup gui
 */

#pragma once

#include "gui/widget.h"

struct osu_game;

namespace oshu {
namespace gui {

/**
 * \ingroup gui
 * \{
 */

struct osu : public widget {
	osu(osu_game &game);
	osu_game &game;
	void draw() override;
};

/** \} */

}}

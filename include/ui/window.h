/**
 * \file include/ui/window.h
 * \ingroup ui_window
 */

#pragma once

#include "ui/audio.h"
#include "ui/background.h"
#include "ui/metadata.h"
#include "ui/score.h"

#include <memory>

struct oshu_game;
struct oshu_game_screen;

namespace oshu {
namespace ui {

struct widget;

/**
 * \defgroup ui_window Window
 * \ingroup ui
 *
 * \brief
 * Manage the game's main window.
 *
 * \{
 */

/**
 * One game window.
 *
 * Multiple game windows are currently not supported, and probably never will.
 */
struct window {
	window(oshu_game&);
	~window();
	/**
	 * The window's associated SDL display.
	 *
	 * It is automatically created when the window is constructed, and
	 * destroyed with the window too.
	 */
	oshu_display *display;
	/**
	 * A reference a game object. It is not owned by the window and must
	 * live longer than the window.
	 */
	oshu_game &game;
	std::shared_ptr<widget> game_view;
	oshu_game_screen *screen;
	oshu_background background {};
	oshu_metadata_frame metadata {};
	oshu_score_frame score {};
	oshu_audio_progress_bar audio_progress_bar {};
};

/**
 * Start the main loop. Stops when the user closes the window, or when the game
 * exits.
 */
void loop(window&);

/** \} */

}}

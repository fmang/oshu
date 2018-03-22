/**
 * \file include/gui/window.h
 * \ingroup gui_window
 */

#pragma once

class oshu_game;

namespace oshu {
namespace gui {

/**
 * \defgroup gui_window Window
 * \ingroup gui
 *
 * \brief
 * Manage the game's main window.
 */

/**
 * One game window.
 *
 * Multiple game windows are currently not supported, and probably never will.
 */
struct window {
	/**
	 * A reference a game object. It is not owned by the window and must
	 * live longer than the window.
	 */
	oshu_game &game;
};

/**
 * Start the main loop. Stops when the user closes the window, or when the game
 * exits.
 */
void loop(window&);

}}

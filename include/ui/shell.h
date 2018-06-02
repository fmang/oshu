/**
 * \file include/ui/shell.h
 * \ingroup ui_shell
 */

#pragma once

#include "ui/audio.h"
#include "ui/background.h"
#include "ui/metadata.h"
#include "ui/score.h"

#include <memory>

namespace oshu {
namespace game {
class base;
}}

struct oshu_game_screen;

namespace oshu {
namespace ui {

struct widget;

/**
 * \defgroup ui_shell Window
 * \ingroup ui
 *
 * \brief
 * Manage the game's main interface.
 *
 * \{
 */

/**
 * The controller of the main game interface.
 *
 * It does not display the game mode itself, but manages every other generic
 * component. The main game engine needs to be set in #game_view.
 */
struct shell {
	/**
	 * Create the shell.
	 *
	 * Both the display and the game must outlive the shell.
	 */
	shell(oshu_display&, oshu::game::base&);
	~shell();
	/**
	 * The window that the shell managed.
	 *
	 * A display should not be associated to more than one shell.
	 */
	oshu_display &display;
	oshu::game::base &game;
	/**
	 * The widget responsible for drawing the main game objects, specific
	 * to the mode.
	 */
	std::unique_ptr<widget> game_view;
	oshu_game_screen *screen;
	oshu_background background {};
	oshu_metadata_frame metadata {};
	oshu_score_frame score {};
	oshu_audio_progress_bar audio_progress_bar {};
	/**
	 * Start the main loop.
	 */
	void open();
	/**
	 * End the game and close the shell.
	 */
	void close();
private:
	/**
	 * When true, the main loop will break.
	 *
	 * \sa close
	 */
	bool stop = false;
};

/** \} */

}}

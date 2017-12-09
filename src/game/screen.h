/**
 * \file game/screen.h
 * \ingroup game_screen
 */

#pragma once

union SDL_Event;

struct oshu_game;

/**
 * \defgroup game_screen Screen
 * \ingroup game
 *
 * A game screen defines the state the game is in.
 *
 * The main idea is that there is the main loop iterating at a constant 60 FPS
 * rate, and handling the common events, like window closing.
 *
 * Then, there are the more specific states. You could imagine a welcome
 * screen, a song selection screen, the actual in-game screen, the pause
 * screen, and score screen.
 *
 * Handling all these various screens in the same loop quickly becomes
 * unreadable. Instead, we'll define a screen by a few entry points, and each
 * screen will be its own master. Most screens would share a more or less
 * common structure, and would share code when appropriate. The main point is
 * that event handlers should never cascade the if-else conditionals depending
 * on the state.
 *
 * \{
 */

/**
 * Define a game screen by its behavior.
 */
struct oshu_game_screen {
	const char *name;
	/**
	 * Called every time an event happens.
	 *
	 * The main game module may preprocess the event, or even filter it.
	 * For instance, when the window is closed.
	 */
	int (*on_event)(struct oshu_game *game, union SDL_Event *event);
	/**
	 * The update function is run at every iteration of the main loop.
	 */
	int (*update)(struct oshu_game *game);
	/**
	 * Draw the screen.
	 *
	 * The screen is cleared before this function is called, and the
	 * display is refreshed after.
	 *
	 * Beside that, no implicit drawing is done for you.
	 */
	int (*draw)(struct oshu_game *game);
};

extern struct oshu_game_screen oshu_play_screen;
extern struct oshu_game_screen oshu_pause_screen;
extern struct oshu_game_screen oshu_score_screen;

/** \} */

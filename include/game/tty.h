/**
 * \file game/tty.h
 * \ingroup game_tty
 */

#pragma once

namespace oshu {

class game_base;

/**
 * \defgroup game_tty TTY
 * \ingroup game
 *
 * \brief
 * Console output.
 *
 * The first oshu! versions didn't print any text information on the game
 * screen, and everything was displayed on the console.
 *
 * Even though the screen would eventually display everything the console
 * shows, it remains a useful tool for debugging and will probably remain here
 * for a long time.
 *
 * For maximum fanciness, this module assumes ANSI color sequences are
 * supported.
 *
 * \todo
 * This tty module is out of place in the game module.
 *
 * \{
 */

/**
 * Welcome the user when the game starts.
 *
 * Show the beatmap's metadata and difficulty information.
 */
void welcome(oshu::game_base *game);

/**
 * Show the state of the game (paused/playing) and the current song position.
 *
 * Only do that for terminal outputs in order not to spam something if the
 * output is redirected.
 *
 * The state length must not decrease over time, otherwise you end up with
 * glitches. If you write `foo\rx`, you get `xoo`. This is the reason the
 * Paused string literal has an extra space.
 */
void print_state(oshu::game_base *game);

/**
 * Congratulate the user when the beatmap is over.
 *
 * Show the number of good hits and bad hits.
 */
void congratulate(oshu::game_base *game);

/** \} */

}

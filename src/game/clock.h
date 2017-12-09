/**
 * \file game/clock.h
 * \ingroup game_clock
 */

#pragma once

/**
 * \defgroup game_clock Clock
 * \ingroup game
 *
 * \{
 */

struct oshu_game;

/**
 * Keep track of various aspects of the elapsing time.
 */
struct oshu_clock {
	/**
	 * The current time in seconds.
	 *
	 * This is the main clock of the game. Use it unless you have a reason
	 * not to.
	 *
	 * 0 is the beginning of the song. It is totally okay for this clock to
	 * be negative when the beatmap has a lead in time.
	 */
	double now;
	/**
	 * Previous time in seconds.
	 *
	 * This is the time at the previous game loop iteration. It is
	 * occasionaly useful to detect when a specific point in time has just
	 * passed.
	 */
	double before;
	/**
	 * The audio clock.
	 *
	 * It may also be accessed directly using the audio module.
	 *
	 * When the audio hasn't started, it sticks at 0.
	 */
	double audio;
	/**
	 * The process time, computed from SDL's ticks.
	 *
	 * This is the reference time when the audio hasn't started, or when it
	 * has stopped.
	 *
	 * It may also be used to increase the accuracy of the game clock,
	 * because sometimes the audio timestamp won't change as no new frame
	 * was decoded.
	 */
	double system;
};

void oshu_initialize_clock(struct oshu_game *game);

/**
 * Update the game clock.
 *
 * It as roughly 2 modes:
 *
 * 1. When the audio has a lead-in time, rely on SDL's ticks to increase the
 *    clock.
 * 2. When the lead-in phase is over, use the audio clock. However, if we
 *    detect it hasn't change, probably because the codec frame is too big, then
 *    we make it progress with the SDL clock anyway.
 *
 * In both cases, we wanna ensure the *now* clock is always monotonous. If we
 * detect the new time is before the previous time, then we stop the time until
 * now catches up with before. That case does happen at least right after the
 * lead-in phase, because the audio starts when the *now* clock becomes
 * positive, while the audio clock will be null at that moment.
 */
void oshu_update_clock(struct oshu_game *game);

/** \} */

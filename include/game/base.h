/**
 * \file game/base.h
 * \ingroup game
 *
 * Define the game module.
 */

#pragma once

#include "audio/audio.h"
#include "audio/library.h"
#include "beatmap/beatmap.h"
#include "game/clock.h"
#include "game/controls.h"
#include "game/mode.h"

namespace oshu {

/**
 * \ingroup game
 *
 * The full game state, from the beatmap state to the audio and graphical
 * context.
 */
class game_base : public game_mode {
public:
	/**
	 * Create the game context for a beatmap, and load all the associated assets.
	 *
	 * After creating the game, you're free to toy with the options of the game,
	 * especially autoplay and pause.
	 *
	 * \todo
	 * It should not be the responsibility of this module to load the beatmap. If
	 * the beatmap is a taiko beatmap, then the taiko game should be instanciated,
	 * not the base module. Instead, take a beatmap by reference.
	 */
	game_base(const char *beatmap_path);
	~game_base();
	/**
	 * Resume the game.
	 *
	 * \sa oshu_pause_game
	 */
	void unpause();
	/**
	 * Pause the game.
	 *
	 * \sa oshu_unpause_game
	 */
	void pause();
	/**
	 * Rewind the song by the specified offset in seconds.
	 *
	 * Rewind the beatmap too but leaving a 1-second break so that we won't seek
	 * right before a note.
	 */
	void rewind(double offset);
	/**
	 * See #rewind.
	 */
	void forward(double offset);
	/**
	 * \todo
	 * Take the beatmap by reference when the game state is constructed.
	 */
	oshu::beatmap beatmap {};
	oshu::audio audio {};
	oshu::sound_library library {};
	oshu::clock clock {};
	int autoplay {};
	bool paused {};
	/**
	 * Pointer to the next clickable hit.
	 *
	 * Any hit before this cursor has already been dealt with, and its
	 * state should be anything but #oshu::INITIAL_HIT.
	 *
	 * On the contrary, any hit past this cursor is *usually* in its
	 * initial state, but that's not an absolute necessity. The user might
	 * click a hit far in the future, but still be able to click the one
	 * right before it.
	 *
	 * This cursor is mostly here to speed things up. It is technically
	 * okay for it to always point to the first hit, but the performance
	 * will suffer. It's ideal position is right at the first *fresh*
	 * (= #oshu::INITIAL_HIT) hit object.
	 *
	 * With the two sentinels in the beatmap's hits linked list, this
	 * cursor is never null, even after the last hit was played.
	 */
	oshu::hit *hit_cursor {};
};

/**
 * \defgroup game_helpers Helpers
 * \ingroup game
 *
 * \brief
 * Read-only game accessors.
 *
 * \{
 */

/**
 * Find the first hit object after *now - offset*.
 *
 * It bases the search on the #oshu::game_base::hit_cursor for performance, but its
 * position won't affect the results.
 *
 * For long notes like sliders, the end time is used, not the start time.
 *
 * \sa oshu::look_hit_up
 */
oshu::hit* look_hit_back(oshu::game_base *game, double offset);

/**
 * Find the last hit object before *now + offset*.
 *
 * This is analogous to #oshu::look_hit_back.
 */
oshu::hit* look_hit_up(oshu::game_base *game, double offset);

/**
 * Return the next relevant hit.
 *
 * A hit is irrelevant when it's not supported by the mode, like sliders.
 *
 * The final null hit is considered relevant in order to ensure this function
 * always return something.
 */
oshu::hit* next_hit(oshu::game_base *game);

/**
 * Like #oshu::next_hit, but in the other direction.
 */
oshu::hit* previous_hit(oshu::game_base *game);

/** \} */

}

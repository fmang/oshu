/**
 * \file game/clock.cc
 * \ingroup game_clock
 */

#include "game/clock.h"

#include "core/log.h"
#include "game/base.h"

void oshu::initialize_clock(oshu::game_base *game)
{
	if (game->beatmap.audio_lead_in > 0.) {
		game->clock.now = - game->beatmap.audio_lead_in;
	} else {
		double first_hit = game->beatmap.hits->next->time;
		if (first_hit < 1.)
			game->clock.now = first_hit - 1.;
	}
	game->clock.system = SDL_GetTicks() / 1000.;
}

/**
 * \todo
 * Find a better way to determine if the music is playing. The clock shouldn't
 * take the game screen into consideration.
 */
void oshu::update_clock(oshu::game_base *game)
{
	oshu::clock *clock = &game->clock;
	double system = SDL_GetTicks() / 1000.;
	double diff = system - clock->system;
	double prev_audio = clock->audio;
	clock->audio = game->audio.music.current_timestamp;
	clock->before = clock->now;
	clock->system = system;

	if (game->paused) {
		/* Don't update the clock when the game is paused. */
	} else if (clock->before < 0) {
		/* Leading in. */
		clock->now = clock->before + diff;
	} else if (clock->audio == prev_audio) {
		/* The audio clock is stuck. */
		/* It happens as the audio buffer is worth several frames. */
		clock->now = clock->before + diff;
	} else {
		/* If the audio clock changed, synchronize the game clock. */
		clock->now = clock->audio;
	}

	/* Force monotonicity. */
	if (clock->now < clock->before)
		clock->now = clock->before;
}

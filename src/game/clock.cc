/**
 * \file game/clock.cc
 * \ingroup game_clock
 */

#include "game/game.h"
#include "game/screens/screens.h"
#include "log.h"

void oshu_initialize_clock(struct oshu_game *game)
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
void oshu_update_clock(struct oshu_game *game)
{
	struct oshu_clock *clock = &game->clock;
	double system = SDL_GetTicks() / 1000.;
	double diff = system - clock->system;
	double prev_audio = clock->audio;
	clock->audio = game->audio.music.current_timestamp;
	clock->before = clock->now;
	clock->system = system;

	if (game->screen == &oshu_pause_screen) {
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

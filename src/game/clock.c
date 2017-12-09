#include "game/game.h"
#include "game/screen.h"

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

void oshu_update_clock(struct oshu_game *game)
{
	struct oshu_clock *clock = &game->clock;
	double system = SDL_GetTicks() / 1000.;
	double diff = system - clock->system;
	double prev_audio = clock->audio;
	clock->audio = game->audio.music.current_timestamp;
	clock->before = clock->now;
	clock->system = system;
	if (game->screen == &oshu_pause_screen)
		; /* don't update the clock */
	else if (clock->before < 0) /* leading in */
		clock->now = clock->before + diff;
	else if (clock->audio == prev_audio) /* audio clock stuck */
		clock->now = clock->before + diff;
	else
		clock->now = clock->audio;
	/* force monotonicity */
	if (clock->now < clock->before)
		clock->now = clock->before;
}

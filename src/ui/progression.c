/**
 * \file ui/progression.c
 * \ingroup ui
 */

#include "game/game.h"

/**
 * Draw at the bottom of the screen a progress bar showing how far in the song
 * we are.
 */
void oshu_show_progression_bar(struct oshu_game *game)
{
	double progression = game->clock.now / game->audio.music.duration;
	if (progression < 0)
		progression = 0;
	else if (progression > 1)
		progression = 1;

	double height = 4;
	SDL_Rect bar = {
		.x = 0,
		.y = cimag(game->display.view.size) - height,
		.w = progression * creal(game->display.view.size),
		.h = height,
	};
	SDL_SetRenderDrawColor(game->display.renderer, 255, 255, 255, 48);
	SDL_SetRenderDrawBlendMode(game->display.renderer, SDL_BLENDMODE_BLEND);
	SDL_RenderFillRect(game->display.renderer, &bar);
}

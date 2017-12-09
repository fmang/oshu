#include "game/game.h"
#include "game/screen.h"

#include <SDL2/SDL.h>

static int on_event(struct oshu_game *game, union SDL_Event *event)
{
	switch (event->type) {
	case SDL_KEYDOWN:
		if (event->key.repeat)
			break;
		switch (event->key.keysym.sym) {
		case OSHU_QUIT_KEY:
			game->state |= OSHU_STOPPING;
			break;
		}
		break;
	case SDL_WINDOWEVENT:
		switch (event->window.event) {
		case SDL_WINDOWEVENT_CLOSE:
			game->state |= OSHU_STOPPING;
			break;
		}
		break;
	}
	return 0;
}

static int update(struct oshu_game *game)
{
	return 0;
}

static int draw(struct oshu_game *game)
{
	SDL_SetRenderDrawColor(game->display.renderer, 0, 0, 0, 255);
	SDL_RenderClear(game->display.renderer);
	game->mode->draw(game);
	SDL_RenderPresent(game->display.renderer);
	return 0;
}

struct oshu_game_screen oshu_score_screen = {
	.on_event = on_event,
	.update = update,
	.draw = draw,
};

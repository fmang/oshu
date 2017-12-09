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
		case OSHU_PAUSE_KEY:
			oshu_pause_game(game);
			break;
		case OSHU_REWIND_KEY:
			oshu_rewind_game(game, 10.);
			break;
		case OSHU_FORWARD_KEY:
			oshu_forward_game(game, 20.);
			break;
		default:
			if (!game->autoplay) {
				enum oshu_finger key = oshu_translate_key(&event->key.keysym);
				if (key != OSHU_UNKNOWN_KEY)
					game->mode->press(game, key);
			}
		}
		break;
	case SDL_KEYUP:
		if (!game->autoplay) {
			enum oshu_finger key = oshu_translate_key(&event->key.keysym);
			if (key != OSHU_UNKNOWN_KEY)
				game->mode->release(game, key);
		}
		break;
	case SDL_MOUSEBUTTONDOWN:
		if (!game->autoplay)
			game->mode->press(game, OSHU_LEFT_BUTTON);
		break;
	case SDL_MOUSEBUTTONUP:
		if (!game->autoplay)
			game->mode->release(game, OSHU_LEFT_BUTTON);
		break;
	case SDL_WINDOWEVENT:
		switch (event->window.event) {
		case SDL_WINDOWEVENT_MINIMIZED:
		case SDL_WINDOWEVENT_FOCUS_LOST:
			if (!game->autoplay && game->hit_cursor->next)
				oshu_pause_game(game);
			break;
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
	return 0;
}

struct oshu_game_screen oshu_play_screen = {
	.on_event = on_event,
	.update = update,
	.draw = draw,
};

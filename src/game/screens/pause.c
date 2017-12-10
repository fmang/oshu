/**
 * \file game/screens/pause.c
 * \ingroup game_screens
 *
 * \brief
 * Implement the pause screen.
 */

#include "game/game.h"
#include "game/screens/screens.h"

#include <SDL2/SDL.h>

static int on_event(struct oshu_game *game, union SDL_Event *event)
{
	switch (event->type) {
	case SDL_KEYDOWN:
		if (event->key.repeat)
			break;
		switch (event->key.keysym.sym) {
		case OSHU_QUIT_KEY:
			oshu_stop_game(game);
			break;
		case OSHU_PAUSE_KEY:
			oshu_unpause_game(game);
			break;
		case OSHU_REWIND_KEY:
			oshu_rewind_game(game, 10.);
			break;
		case OSHU_FORWARD_KEY:
			oshu_forward_game(game, 20.);
			break;
		}
		break;
	case SDL_WINDOWEVENT:
		switch (event->window.event) {
		case SDL_WINDOWEVENT_CLOSE:
			oshu_stop_game(game);
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

static void draw_pause(struct oshu_display *display)
{
	const double size = 100;
	const double thickness = 40;
	const oshu_size screen = display->view.size;
	SDL_SetRenderDrawColor(display->renderer, 255, 255, 255, 128);
	SDL_SetRenderDrawBlendMode(display->renderer, SDL_BLENDMODE_BLEND);
	SDL_Rect bar = {
		.x = creal(screen) / 2. - size / 2.,
		.y = cimag(screen) / 2. - size / 2.,
		.w = thickness,
		.h = size,
	};
	SDL_RenderFillRect(display->renderer, &bar);
	bar.x += size - thickness;
	SDL_RenderFillRect(display->renderer, &bar);
}

static int draw(struct oshu_game *game)
{
	oshu_show_background(game);
	oshu_show_metadata(game);
	oshu_show_progression_bar(game);
	draw_pause(&game->display);
	return 0;
}

struct oshu_game_screen oshu_pause_screen = {
	.name = "Paused",
	.on_event = on_event,
	.update = update,
	.draw = draw,
};

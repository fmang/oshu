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

/**
 * Print the score if the song was finished, then exit.
 */
static void end(struct oshu_game *game)
{
	/* Clear the status line. */
	printf("\r                                        \r");
	/* Compute the score. */
	int good = 0;
	int missed = 0;
	for (struct oshu_hit *hit = game->beatmap.hits; hit; hit = hit->next) {
		if (hit->state == OSHU_MISSED_HIT)
			missed++;
		else if (hit->state == OSHU_GOOD_HIT)
			good++;
	}
	double rate = (double) good / (good + missed);
	printf(
		"  \033[1mScore:\033[0m\n"
		"  \033[%dm%3d\033[0m good\n"
		"  \033[%dm%3d\033[0m miss\n"
		"\n",
		rate >= 0.9 ? 32 : 0, good,
		rate < 0.5  ? 31 : 0, missed
	);
}

static void check_end(struct oshu_game *game)
{
	if (game->hit_cursor->next)
		return;
	if (game->clock.now > oshu_hit_end_time(game->hit_cursor->previous) + game->beatmap.difficulty.leniency) {
		game->state = OSHU_FINISHED | OSHU_PLAYING;
		game->screen = &oshu_score_screen;
		end(game);
	}
}

static int update(struct oshu_game *game)
{
	if (game->clock.now >= 0)
		oshu_play_audio(&game->audio);
	if (game->autoplay)
		game->mode->autoplay(game);
	else
		game->mode->check(game);
	oshu_print_state(game);
	check_end(game);
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

struct oshu_game_screen oshu_play_screen = {
	.on_event = on_event,
	.update = update,
	.draw = draw,
};

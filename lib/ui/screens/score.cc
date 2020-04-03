/**
 * \file lib/ui/screens/score.cc
 * \ingroup ui_screens
 *
 * \brief
 * Implement the score screen, when the game is finished.
 */

#include "./screens.h"

#include "game/base.h"
#include "ui/shell.h"
#include "video/transitions.h"

#include <SDL2/SDL.h>

static int on_event(oshu::shell &w, union SDL_Event *event)
{
	switch (event->type) {
	case SDL_KEYDOWN:
		if (event->key.repeat)
			break;
		switch (event->key.keysym.sym) {
		case oshu::QUIT_KEY:
			w.close();
			break;
		}
		break;
	case SDL_WINDOWEVENT:
		switch (event->window.event) {
		case SDL_WINDOWEVENT_CLOSE:
			w.close();
			break;
		}
		break;
	}
	return 0;
}

static int update(oshu::shell &w)
{
	return 0;
}

static int draw(oshu::shell &w)
{
	oshu::game_base *game = &w.game;
	SDL_ShowCursor(SDL_ENABLE);
	double end = oshu::hit_end_time(oshu::previous_hit(game));
	double r = oshu::fade_in(end + 1, end + 2, game->clock.now);
	oshu::show_background(&w.background, r);
	oshu::show_audio_progress_bar(&w.audio_progress_bar);
	oshu::show_metadata_frame(&w.metadata, r);
	oshu::show_score_frame(&w.score, r);
	return 0;
}

/**
 * Game complete screen.
 *
 * Once this screen is reached, the only command left is *exit*.
 *
 */
oshu::game_screen oshu::score_screen = {
	.name = "Finished",
	.on_event = on_event,
	.update = update,
	.draw = draw,
};

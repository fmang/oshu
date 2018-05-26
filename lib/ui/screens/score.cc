/**
 * \file lib/ui/screens/score.cc
 * \ingroup ui_screens
 *
 * \brief
 * Implement the score screen, when the game is finished.
 */

#include "./screens.h"

#include "game/base.h"
#include "ui/window.h"
#include "video/transitions.h"

#include <SDL2/SDL.h>

static int on_event(oshu::ui::window &w, union SDL_Event *event)
{
	oshu::game::base *game = &w.game;
	switch (event->type) {
	case SDL_KEYDOWN:
		if (event->key.repeat)
			break;
		switch (event->key.keysym.sym) {
		case OSHU_QUIT_KEY:
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

static int update(oshu::ui::window &w)
{
	return 0;
}

static int draw(oshu::ui::window &w)
{
	oshu::game::base *game = &w.game;
	SDL_ShowCursor(SDL_ENABLE);
	double end = oshu_hit_end_time(oshu_previous_hit(game));
	double r = oshu_fade_in(end + 1, end + 2, game->clock.now);
	oshu_show_background(&w.background, r);
	oshu_show_audio_progress_bar(&w.audio_progress_bar);
	oshu_show_metadata_frame(&w.metadata, r);
	oshu_show_score_frame(&w.score, r);
	return 0;
}

/**
 * Game complete screen.
 *
 * Once this screen is reached, the only command left is *exit*.
 *
 */
struct oshu_game_screen oshu_score_screen = {
	.name = "Finished",
	.on_event = on_event,
	.update = update,
	.draw = draw,
};

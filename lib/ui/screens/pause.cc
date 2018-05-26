/**
 * \file lib/ui/screens/pause.cc
 * \ingroup ui_screens
 *
 * \brief
 * Implement the pause screen.
 */

#include "./screens.h"

#include "game/base.h"
#include "ui/window.h"
#include "video/display.h"

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
			game->stop = true;
			break;
		case OSHU_PAUSE_KEY:
			if (game->clock.now > 0 && !game->autoplay)
				game->rewind(1.);
			game->unpause();
			w.screen = &oshu_play_screen;
			break;
		case OSHU_REWIND_KEY:
			game->rewind(10.);
			break;
		case OSHU_FORWARD_KEY:
			game->forward(20.);
			break;
		}
		break;
	case SDL_WINDOWEVENT:
		switch (event->window.event) {
		case SDL_WINDOWEVENT_CLOSE:
			game->stop = true;
			break;
		}
		break;
	}
	return 0;
}

static int update(oshu::ui::window &w)
{
	oshu::game::base *game = &w.game;
	if (!game->paused)
		w.screen = &oshu_play_screen;
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
		.x = (int) (std::real(screen) / 2. - size / 2.),
		.y = (int) (std::imag(screen) / 2. - size / 2.),
		.w = (int) thickness,
		.h = (int) size,
	};
	SDL_RenderFillRect(display->renderer, &bar);
	bar.x += size - thickness;
	SDL_RenderFillRect(display->renderer, &bar);
}

static int draw(oshu::ui::window &w)
{
	oshu::game::base *game = &w.game;
	SDL_ShowCursor(SDL_ENABLE);
	oshu_show_background(&w.background, 0);
	oshu_show_metadata_frame(&w.metadata, 1);
	oshu_show_audio_progress_bar(&w.audio_progress_bar);
	draw_pause(w.display);
	return 0;
}

/**
 * Pause screen: the music stops.
 *
 * In the pause screen, the user cannot interact with the game itself, but may
 * control the audio position by seeking.
 *
 * The beatmap isn't drawn, but instead a pause symbol and the beatmap's
 * metadata are shown.
 *
 * When resuming, the audio is rewinded by 1 second to leave the user some time
 * to re-focus.
 *
 */
struct oshu_game_screen oshu_pause_screen = {
	.name = "Paused",
	.on_event = on_event,
	.update = update,
	.draw = draw,
};

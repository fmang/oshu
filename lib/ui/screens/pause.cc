/**
 * \file lib/ui/screens/pause.cc
 * \ingroup ui_screens
 *
 * \brief
 * Implement the pause screen.
 */

#include "./screens.h"

#include "game/base.h"
#include "ui/shell.h"
#include "video/display.h"

#include <SDL2/SDL.h>

static int on_event(oshu::shell &w, union SDL_Event *event)
{
	oshu::game_base *game = &w.game;
	switch (event->type) {
	case SDL_KEYDOWN:
		if (event->key.repeat)
			break;
		switch (event->key.keysym.sym) {
		case oshu::QUIT_KEY:
			w.close();
			break;
		case oshu::PAUSE_KEY:
			if (game->clock.now > 0 && !game->autoplay)
				game->rewind(1.);
			game->unpause();
			w.screen = &oshu::play_screen;
			break;
		case oshu::REWIND_KEY:
			game->rewind(10.);
			break;
		case oshu::FORWARD_KEY:
			game->forward(20.);
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
	oshu::game_base *game = &w.game;
	if (!game->paused)
		w.screen = &oshu::play_screen;
	return 0;
}

static void draw_pause(struct oshu::display *display)
{
	const double size = 100;
	const double thickness = 40;
	const oshu::size screen = display->view.size;
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

static int draw(oshu::shell &w)
{
	oshu::game_base *game = &w.game;
	SDL_ShowCursor(SDL_ENABLE);
	oshu::show_background(&w.background, 0);
	oshu::show_metadata_frame(&w.metadata, 1);
	oshu::show_audio_progress_bar(&w.audio_progress_bar);
	draw_pause(&w.display);
	return 0;
}

namespace oshu {

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
struct oshu::game_screen pause_screen = {
	.name = "Paused",
	.on_event = on_event,
	.update = update,
	.draw = draw,
};

}

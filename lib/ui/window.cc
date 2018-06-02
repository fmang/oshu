/**
 * \file lib/ui/window.cc
 * \ingroup ui_window
 */

#include "ui/window.h"

#include "game/base.h"
#include "core/log.h"
#include "game/tty.h"
#include "ui/widget.h"
#include "video/display.h"

#include "./screens/screens.h"

static void open_display(oshu::ui::window &w)
{
	w.display = new oshu_display;
	struct oshu_metadata *meta = &w.game.beatmap.metadata;
	std::ostringstream title;
	title << meta->artist << " - " << meta->title << " ♯ " << meta->version << " 𝄞 oshu!";
	SDL_SetWindowTitle(w.display->window, title.str().c_str());
	oshu_reset_view(w.display);
}

namespace oshu {
namespace ui {

window::window(oshu::game::base &game)
: game(game), screen(&oshu_play_screen)
{
	open_display(*this);
	if (game.beatmap.background_filename)
		oshu_load_background(display, game.beatmap.background_filename, &background);
	oshu_create_metadata_frame(display, &game.beatmap, &game.clock.system, &metadata);
	oshu_create_audio_progress_bar(display, &game.audio.music, &audio_progress_bar);
}

window::~window()
{
	game_view.release();
	oshu_destroy_background(&background);
	oshu_destroy_metadata_frame(&metadata);
	oshu_destroy_score_frame(&score);
	oshu_destroy_audio_progress_bar(&audio_progress_bar);
	delete display;
}

static void draw(window &w)
{
	oshu::game::base *game = &w.game;
	SDL_SetRenderDrawColor(w.display->renderer, 0, 0, 0, 255);
	SDL_RenderClear(w.display->renderer);
	w.screen->draw(w);
	SDL_RenderPresent(w.display->renderer);
}

void window::open()
{
	oshu_welcome(&game);
	oshu_initialize_clock(&game);

	SDL_Event event;
	int missed_frames = 0;

	while (!stop) {
		oshu_update_clock(&game);
		oshu_reset_view(display);
		while (SDL_PollEvent(&event))
			screen->on_event(*this, &event);
		screen->update(*this);
		draw(*this);

		/* Calling oshu_print_state before draw causes some flickering
		 * on the tty, for some reason. */
		if (screen == &oshu_play_screen)
			oshu_print_state(&game);

		double advance = display->frame_duration - (SDL_GetTicks() / 1000. - game.clock.system);
		if (advance > 0) {
			SDL_Delay(advance * 1000);
		} else {
			missed_frames++;
			if (missed_frames == 1000) {
				oshu_log_warning("your computer is having a hard time keeping up");
				if (display->features)
					oshu_log_warning("try running oshu! with OSHU_QUALITY=low (see the man page)");
			}
		}
	}

	if (screen != &oshu_score_screen)
		puts("");
		/* write a new line to avoid conflict between the status line
		 * and the shell prompt */
	oshu_log_debug("%d missed frames", missed_frames);
}

void window::close()
{
	stop = true;
}

}}

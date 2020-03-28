/**
 * \file lib/ui/shell.cc
 * \ingroup ui_shell
 */

#include "ui/shell.h"

#include "game/base.h"
#include "core/log.h"
#include "game/tty.h"
#include "ui/widget.h"
#include "video/display.h"

#include "./screens/screens.h"

static void set_title(oshu::shell &w)
{
	oshu::metadata *meta = &w.game.beatmap.metadata;
	std::ostringstream title;
	title << meta->artist << " - " << meta->title << " ♯ " << meta->version << " 𝄞 oshu!";
	SDL_SetWindowTitle(w.display.window, title.str().c_str());
	oshu::reset_view(&w.display);
}

namespace oshu {

shell::shell(oshu::display &display, oshu::game_base &game)
: display(display), game(game), screen(&oshu::play_screen)
{
	set_title(*this);
	if (game.beatmap.background_filename)
		oshu::load_background(&display, game.beatmap.background_filename, &background);
	oshu::create_metadata_frame(&display, &game.beatmap, &game.clock.system, &metadata);
	oshu::create_audio_progress_bar(&display, &game.audio.music, &audio_progress_bar);
}

shell::~shell()
{
	game_view.release();
	oshu::destroy_background(&background);
	oshu::destroy_metadata_frame(&metadata);
	oshu::destroy_score_frame(&score);
	oshu::destroy_audio_progress_bar(&audio_progress_bar);
}

static void draw(shell &w)
{
	oshu::game_base *game = &w.game;
	SDL_SetRenderDrawColor(w.display.renderer, 0, 0, 0, 255);
	SDL_RenderClear(w.display.renderer);
	w.screen->draw(w);
	SDL_RenderPresent(w.display.renderer);
}

void shell::open()
{
	oshu::welcome(&game);
	oshu::initialize_clock(&game);

	SDL_Event event;
	int missed_frames = 0;

	while (!stop) {
		oshu::update_clock(&game);
		oshu::reset_view(&display);
		while (SDL_PollEvent(&event))
			screen->on_event(*this, &event);
		screen->update(*this);
		draw(*this);

		/* Calling oshu::print_state before draw causes some flickering
		 * on the tty, for some reason. */
		if (screen == &oshu::play_screen)
			oshu::print_state(&game);

		double advance = display.frame_duration - (SDL_GetTicks() / 1000. - game.clock.system);
		if (advance > 0) {
			SDL_Delay(advance * 1000);
		} else {
			missed_frames++;
			if (missed_frames == 1000) {
				oshu_log_warning("your computer is having a hard time keeping up");
				if (display.features)
					oshu_log_warning("try running oshu! with OSHU_QUALITY=low (see the man page)");
			}
		}
	}

	if (screen != &oshu::score_screen)
		puts("");
		/* write a new line to avoid conflict between the status line
		 * and the shell prompt */
	oshu_log_debug("%d missed frames", missed_frames);
}

void shell::close()
{
	stop = true;
}

}

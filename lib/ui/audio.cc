/**
 * \file ui/audio.cc
 * \ingroup ui_audio
 */

#include "ui/audio.h"

#include "audio/stream.h"
#include "video/display.h"

#include <assert.h>
#include <SDL2/SDL.h>

int oshu::create_audio_progress_bar(struct oshu::display *display, struct oshu::stream *stream, struct oshu::audio_progress_bar *bar)
{
	bar->display = display;
	bar->stream = stream;
	return 0;
}

void oshu::show_audio_progress_bar(struct oshu::audio_progress_bar *bar)
{
	assert (bar->stream->duration != 0);
	double progression = bar->stream->current_timestamp / bar->stream->duration;
	if (progression < 0)
		progression = 0;
	else if (progression > 1)
		progression = 1;

	double height = 4;
	SDL_Rect shape = {
		.x = 0,
		.y = (int) (std::imag(bar->display->view.size) - height),
		.w = (int) (progression * std::real(bar->display->view.size)),
		.h = (int) height,
	};
	SDL_SetRenderDrawColor(bar->display->renderer, 255, 255, 255, 48);
	SDL_SetRenderDrawBlendMode(bar->display->renderer, SDL_BLENDMODE_BLEND);
	SDL_RenderFillRect(bar->display->renderer, &shape);
}

void oshu::destroy_audio_progress_bar(struct oshu::audio_progress_bar *bar)
{
}

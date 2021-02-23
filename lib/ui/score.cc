/**
 * \file ui/score.cc
 * \ingroup ui_score
 */

#include "config.h"

#include "ui/score.h"

#include "beatmap/beatmap.h"
#include "video/display.h"

#include <SDL2/SDL.h>

int oshu::create_score_frame(oshu::display *display, oshu::beatmap *beatmap, oshu::score_frame *frame)
{
	memset(frame, 0, sizeof(*frame));
	frame->display = display;
	frame->beatmap = beatmap;
	frame->score = oshu::score(beatmap);
	return 0;
}

void oshu::show_score_frame(oshu::score_frame *frame, double opacity)
{
	if (std::isnan(frame->score)) return;
	SDL_SetRenderDrawBlendMode(frame->display->renderer, SDL_BLENDMODE_BLEND);

	SDL_Rect bar = {
		.x = (int) (std::real(frame->display->view.size) * 0.15),
		.y = (int) (std::imag(frame->display->view.size) - 15),
		.w = (int) (std::real(frame->display->view.size) * 0.70),
		.h = 5,
	};

	SDL_Rect good = {
		.x = bar.x,
		.y = bar.y,
		.w = (int) ((double) frame->score * bar.w),
		.h = bar.h,
	};
	SDL_SetRenderDrawColor(frame->display->renderer, 0, 255, 0, 196 * opacity);
	SDL_RenderFillRect(frame->display->renderer, &good);

	SDL_Rect bad = {
		.x = good.x + good.w,
		.y = good.y,
		.w = bar.w - good.w,
		.h = good.h,
	};
	SDL_SetRenderDrawColor(frame->display->renderer, 255, 0, 0, 196 * opacity);
	SDL_RenderFillRect(frame->display->renderer, &bad);
}

void oshu::destroy_score_frame(oshu::score_frame *frame)
{
}

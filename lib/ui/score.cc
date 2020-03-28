/**
 * \file ui/score.cc
 * \ingroup ui_score
 */

#include "config.h"

#include "ui/score.h"

#include "beatmap/beatmap.h"
#include "video/display.h"

#include <SDL2/SDL.h>

int oshu::create_score_frame(struct oshu::display *display, struct oshu::beatmap *beatmap, struct oshu::score_frame *frame)
{
	memset(frame, 0, sizeof(*frame));
	frame->display = display;
	frame->beatmap = beatmap;

	for (struct oshu::hit *hit = beatmap->hits; hit; hit = hit->next) {
		if (hit->state == oshu::MISSED_HIT)
			++frame->bad;
		else if (hit->state == oshu::GOOD_HIT)
			++frame->good;
	}

	return 0;
}

void oshu::show_score_frame(struct oshu::score_frame *frame, double opacity)
{
	int notes = frame->good + frame->bad;
	if (notes == 0)
		return;

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
		.w = (int) ((double) frame->good / notes * bar.w),
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

void oshu::destroy_score_frame(struct oshu::score_frame *frame)
{
}

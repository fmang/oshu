/**
 * \file ui/score.c
 * \ingroup ui_score
 */

#include "../config.h"

#include "ui/score.h"

#include "beatmap/beatmap.h"
#include "graphics/display.h"
#include "graphics/paint.h"
#include "graphics/texture.h"

#include <assert.h>
#include <string.h>
#include <SDL2/SDL.h>

/**
 * Estimate the duration of a beatmap from the timing information of its last
 * hit object.
 */
double beatmap_duration(struct oshu_beatmap *beatmap)
{
	struct oshu_hit *hit = beatmap->hits;
	while (hit->next)
		hit = hit->next;
	return oshu_hit_end_time(hit->previous);
}

static int paint(struct oshu_score_widget *widget)
{
	oshu_size size = 100 + 600 * I;
	struct oshu_painter p;
	oshu_start_painting(widget->display, size, &p);
	cairo_translate(p.cr, creal(size) / 2., 0);

	cairo_set_source_rgba(p.cr, 0, 0, 0, .6);
	cairo_set_line_width(p.cr, 2);
	cairo_move_to(p.cr, 0, 0);
	cairo_line_to(p.cr, 0, cimag(size));
	cairo_stroke(p.cr);

	double duration = beatmap_duration(widget->beatmap);
	double leniency = widget->beatmap->difficulty.leniency;
	assert (leniency > 0);
	assert (duration > 0);

	cairo_set_source_rgba(p.cr, 1, 0, 0, .6);
	cairo_set_line_width(p.cr, 1);
	for (struct oshu_hit *hit = widget->beatmap->hits; hit; hit = hit->next) {
		if (hit->state == OSHU_MISSED_HIT) {
			double y = hit->time / duration * cimag(size);
			cairo_move_to(p.cr, -10, y);
			cairo_line_to(p.cr, 10, y);
			cairo_stroke(p.cr);
		}
	}

	cairo_set_source_rgba(p.cr, 1, 1, 0, .6);
	cairo_set_line_width(p.cr, 1);
	cairo_move_to(p.cr, 0, 0);
	for (struct oshu_hit *hit = widget->beatmap->hits; hit; hit = hit->next) {
		if (hit->offset < 0) {
			double y = hit->time / duration * cimag(size);
			double x = hit->offset / leniency * creal(size) / 2;
			cairo_line_to(p.cr, x, y);
		}
	}
	cairo_line_to(p.cr, 0, cimag(size));
	cairo_stroke(p.cr);

	cairo_set_source_rgba(p.cr, 1, 0, 1, .6);
	cairo_set_line_width(p.cr, 1);
	cairo_move_to(p.cr, 0, 0);
	for (struct oshu_hit *hit = widget->beatmap->hits; hit; hit = hit->next) {
		if (hit->offset > 0) {
			double y = hit->time / duration * cimag(size);
			double x = hit->offset / leniency * creal(size) / 2;
			cairo_line_to(p.cr, x, y);
		}
	}
	cairo_line_to(p.cr, 0, cimag(size));
	cairo_stroke(p.cr);

	struct oshu_texture *texture = &widget->offset_graph;
	int rc = oshu_finish_painting(&p, texture);
	texture->origin = size / 2.;
	return rc;
}

int oshu_create_score_widget(struct oshu_display *display, struct oshu_beatmap *beatmap, struct oshu_score_widget *widget)
{
	memset(widget, 0, sizeof(*widget));
	widget->display = display;
	widget->beatmap = beatmap;
	return paint(widget);
}

void oshu_show_score_widget(struct oshu_score_widget *widget)
{
	double x = creal(widget->display->view.size) - creal(widget->offset_graph.size);
	double y = cimag(widget->display->view.size) / 2;
	oshu_draw_texture(widget->display, &widget->offset_graph, x + y * I);
}

void oshu_destroy_score_widget(struct oshu_score_widget *widget)
{
	oshu_destroy_texture(&widget->offset_graph);
}

int oshu_create_score_frame(struct oshu_display *display, struct oshu_beatmap *beatmap, struct oshu_score_frame *frame)
{
	memset(frame, 0, sizeof(*frame));
	frame->display = display;
	frame->beatmap = beatmap;

	for (struct oshu_hit *hit = beatmap->hits; hit; hit = hit->next) {
		if (hit->state == OSHU_MISSED_HIT)
			++frame->bad;
		else if (hit->state == OSHU_GOOD_HIT)
			++frame->good;
	}

	return 0;
}

void oshu_show_score_frame(struct oshu_score_frame *frame, double opacity)
{
	int notes = frame->good + frame->bad;
	if (notes == 0)
		return;

	SDL_SetRenderDrawBlendMode(frame->display->renderer, SDL_BLENDMODE_BLEND);

	SDL_Rect bar = {
		.x = creal(frame->display->view.size) * 0.15,
		.y = cimag(frame->display->view.size) - 15,
		.w = creal(frame->display->view.size) * 0.70,
		.h = 5,
	};

	SDL_Rect good = {
		.x = bar.x,
		.y = bar.y,
		.w = (double) frame->good / notes * bar.w,
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

void oshu_destroy_score_frame(struct oshu_score_frame *frame)
{
}

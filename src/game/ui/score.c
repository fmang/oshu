/**
 * \file game/ui/score.c
 * \ingroup game_ui
 */

#include "../config.h"

#include "game/game.h"
#include "graphics/paint.h"
#include "graphics/texture.h"

#include <assert.h>

int oshu_paint_score(struct oshu_game *game)
{
	oshu_size size = 100 + 600 * I;

	struct oshu_painter p;
	oshu_start_painting(&game->display, size, &p);
	cairo_translate(p.cr, creal(size) / 2., 0);

	cairo_set_source_rgba(p.cr, 0, 0, 0, .6);
	cairo_set_line_width(p.cr, 2);
	cairo_move_to(p.cr, 0, 0);
	cairo_line_to(p.cr, 0, cimag(size));
	cairo_stroke(p.cr);

	double duration = game->audio.music.duration;
	double leniency = game->beatmap.difficulty.leniency;
	assert (leniency > 0);
	assert (duration > 0);

	cairo_set_source_rgba(p.cr, 1, 0, 0, .6);
	cairo_set_line_width(p.cr, 1);
	for (struct oshu_hit *hit = game->beatmap.hits; hit; hit = hit->next) {
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
	for (struct oshu_hit *hit = game->beatmap.hits; hit; hit = hit->next) {
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
	for (struct oshu_hit *hit = game->beatmap.hits; hit; hit = hit->next) {
		if (hit->offset > 0) {
			double y = hit->time / duration * cimag(size);
			double x = hit->offset / leniency * creal(size) / 2;
			cairo_line_to(p.cr, x, y);
		}
	}
	cairo_line_to(p.cr, 0, cimag(size));
	cairo_stroke(p.cr);

	struct oshu_texture *texture = &game->ui.score.offset_graph;
	int rc = oshu_finish_painting(&p, texture);
	texture->origin = size / 2.;
	return rc;
}

void oshu_show_score(struct oshu_game *game)
{
	double x = creal(game->display.view.size) - creal(game->ui.score.offset_graph.size);
	double y = cimag(game->display.view.size) / 2;
	oshu_draw_texture(&game->display, &game->ui.score.offset_graph, x + y * I);
}

void oshu_free_score(struct oshu_game *game)
{
	oshu_destroy_texture(&game->ui.score.offset_graph);
}

/**
 * \file game/osu/paint.h
 * \ingroup osu
 */

#include "game/game.h"
#include "graphics/paint.h"

#include "log.h"

static int paint_circle(struct oshu_game *game)
{
	double radius = game->beatmap.difficulty.circle_radius;
	oshu_size size = (1. + I) *  radius * 2.;
	double zoom = 3;

	struct oshu_painter p;
	oshu_start_painting(size * zoom, &p);
	cairo_translate(p.cr, radius * zoom, radius * zoom);

	cairo_set_source_rgba(p.cr, 1, 1, 1, .7);
	cairo_arc(p.cr, 0, 0, radius * zoom, 0, 2. * M_PI);
	cairo_fill(p.cr);

	cairo_set_source_rgba(p.cr, .1, .1, .1, 1);
	cairo_set_line_width(p.cr, 3 * zoom);
	cairo_arc(p.cr, 0, 0, (radius - 4) * zoom, 0, 2. * M_PI);
	cairo_stroke(p.cr);

	int rc = oshu_finish_painting(&p, &game->display, &game->osu.circle_texture);
	game->osu.circle_texture.size = size;
	game->osu.circle_texture.origin = size / 2.;
	return rc;
}

int osu_paint_resources(struct oshu_game *game)
{
	return paint_circle(game);
}

int osu_free_resources(struct oshu_game *game)
{
	oshu_destroy_texture(&game->osu.circle_texture);
	return 0;
}

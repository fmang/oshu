/**
 * \file game/osu/paint.h
 * \ingroup osu
 */

#include "game/game.h"
#include "graphics/paint.h"

static int paint_circle(struct oshu_game *game)
{
	struct oshu_painter p;
	oshu_start_painting(256 + 256 * I, &p);
	cairo_translate(p.cr, 128, 128);

	cairo_set_source_rgba(p.cr, 1, 1, 1, 1);
	cairo_set_line_width(p.cr, 8);
	cairo_arc(p.cr, 0, 0, 124, 0, 2. * M_PI);
	cairo_stroke(p.cr);

	int rc = oshu_finish_painting(&p, &game->display, &game->osu.circle_texture);
	game->osu.circle_texture.size = (1. + I) * game->beatmap.difficulty.circle_radius * 2.;
	game->osu.circle_texture.origin = game->osu.circle_texture.size / 2.;
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

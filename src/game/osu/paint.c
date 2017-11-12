/**
 * \file game/osu/paint.h
 * \ingroup osu
 */

#include "game/game.h"
#include "graphics/paint.h"

int osu_paint_resources(struct oshu_game *game)
{
	/* XXX Toying around */
	struct oshu_painter p;
	oshu_start_painting(400 + 400 * I, 128 + 128 * I, &p);
	cairo_t *cr = p.cr;

	double xc = 0.0;
	double yc = 0.0;
	double radius = 100.0;
	double angle1 = 45.0  * (M_PI/180.0);  /* angles are specified */
	double angle2 = 180.0 * (M_PI/180.0);  /* in radians           */

	cairo_set_line_width (cr, 10.0);
	cairo_arc (cr, xc, yc, radius, angle1, angle2);
	cairo_stroke (cr);

	/* draw helping lines */
	cairo_set_source_rgba (cr, 1, 0.2, 0.2, 0.6);
	cairo_set_line_width (cr, 6.0);

	cairo_arc (cr, xc, yc, 10.0, 0, 2*M_PI);
	cairo_fill (cr);

	cairo_arc (cr, xc, yc, radius, angle1, angle1);
	cairo_line_to (cr, xc, yc);
	cairo_arc (cr, xc, yc, radius, angle2, angle2);
	cairo_line_to (cr, xc, yc);
	cairo_stroke (cr);

	oshu_finish_painting(&p, &game->display, &game->osu.circle_texture);
	return 0;
}

int osu_free_resources(struct oshu_game *game)
{
	oshu_destroy_texture(&game->osu.circle_texture);
	return 0;
}

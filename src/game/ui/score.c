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
	oshu_size size = 400 + 200 * I;

	struct oshu_painter p;
	oshu_start_painting(&game->display, size, &p);
	cairo_translate(p.cr, 0, cimag(size) / 2.);

	cairo_set_source_rgba(p.cr, 0, 0, 0, 1);
	cairo_move_to(p.cr, 0, 0);
	cairo_line_to(p.cr, creal(size), 0);
	cairo_stroke(p.cr);

	struct oshu_texture *texture = &game->ui.score.offset_graph;
	int rc = oshu_finish_painting(&p, texture);
	texture->origin = size / 2.;
	return rc;
}

void oshu_show_score(struct oshu_game *game)
{
	oshu_draw_texture(&game->display, &game->ui.score.offset_graph, game->display.view.size / 2.);
}

void oshu_free_score(struct oshu_game *game)
{
	oshu_destroy_texture(&game->ui.score.offset_graph);
}

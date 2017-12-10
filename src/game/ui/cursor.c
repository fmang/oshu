/**
 * \file game/ui/cursor.c
 * \ingroup game_ui
 */

#include "game/game.h"
#include "graphics/paint.h"

int oshu_paint_cursor(struct oshu_game *game)
{
	double radius = 20;
	oshu_size size = (1. + I) * radius * 2.;

	struct oshu_painter p;
	oshu_start_painting(&game->display, size, &p);
	cairo_translate(p.cr, radius, radius);

	cairo_pattern_t *pattern = cairo_pattern_create_radial(
		0, 0, 0,
		0, 0, radius - 1
	);
	cairo_pattern_add_color_stop_rgba(pattern, 0.0, 1, 1, 1, .8);
	cairo_pattern_add_color_stop_rgba(pattern, 0.6, 1, 1, 1, .8);
	cairo_pattern_add_color_stop_rgba(pattern, 0.7, 1, 1, 1, .3);
	cairo_pattern_add_color_stop_rgba(pattern, 1.0, 1, 1, 1, 0);
	cairo_arc(p.cr, 0, 0, radius - 1, 0, 2. * M_PI);
	cairo_set_source(p.cr, pattern);
	cairo_fill(p.cr);
	cairo_pattern_destroy(pattern);

	struct oshu_texture *texture = &game->ui.cursor.mouse;
	int rc = oshu_finish_painting(&p, texture);
	texture->origin = size / 2.;
	return rc;
}

void oshu_show_cursor(struct oshu_game *game)
{
	int fireflies = sizeof(game->ui.cursor.history) / sizeof(*game->ui.cursor.history);
	game->ui.cursor.offset = (game->ui.cursor.offset + 1) % fireflies;
	game->ui.cursor.history[game->ui.cursor.offset] = oshu_get_mouse(&game->display);

	for (int i = 1; i <= fireflies; ++i) {
		int offset = (game->ui.cursor.offset + i) % fireflies;
		double ratio = (double) (i + 1) / (fireflies + 1);
		SDL_SetTextureAlphaMod(game->ui.cursor.mouse.texture, ratio * 255);
		oshu_draw_scaled_texture(
			&game->display, &game->ui.cursor.mouse,
			game->ui.cursor.history[offset],
			ratio
		);
	}
}

void oshu_free_cursor(struct oshu_game *game)
{
	oshu_destroy_texture(&game->ui.cursor.mouse);
}

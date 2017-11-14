/**
 * \file game/osu/paint.h
 * \ingroup osu
 */

#include "game/game.h"
#include "graphics/paint.h"
#include "log.h"

#include <assert.h>

static int paint_circle(struct oshu_game *game)
{
	double radius = game->beatmap.difficulty.circle_radius;
	oshu_size size = (1. + I) *  radius * 2.;
	double zoom = 3;

	struct oshu_painter p;
	oshu_start_painting(size * zoom, &p);
	cairo_scale(p.cr, zoom, zoom);
	cairo_translate(p.cr, radius, radius);

	cairo_set_source_rgba(p.cr, 1, 1, 1, .7);
	cairo_arc(p.cr, 0, 0, radius, 0, 2. * M_PI);
	cairo_fill(p.cr);

	cairo_set_source_rgba(p.cr, .1, .1, .1, 1);
	cairo_set_line_width(p.cr, 3);
	cairo_arc(p.cr, 0, 0, radius - 4, 0, 2. * M_PI);
	cairo_stroke(p.cr);

	int rc = oshu_finish_painting(&p, &game->display, &game->osu.circle_texture);
	game->osu.circle_texture.size = size;
	game->osu.circle_texture.origin = size / 2.;
	return rc;
}

static int paint_slider(struct oshu_game *game, struct oshu_hit *hit)
{
	assert (hit->type & OSHU_SLIDER_HIT);
	oshu_vector radius = game->beatmap.difficulty.circle_radius * (1. + I);
	oshu_point top_left, bottom_right;
	oshu_path_bounding_box(&hit->slider.path, &top_left, &bottom_right);
	oshu_size size = bottom_right - top_left + 2. * radius;
	double zoom = 2;

	struct oshu_painter p;
	oshu_start_painting(size * zoom, &p);

	cairo_set_source_rgba(p.cr, 1, 0, 0, 1);
	cairo_set_line_width(p.cr, 5);
	cairo_rectangle(p.cr, 0, 0, creal(size) * zoom, cimag(size) * zoom);
	cairo_stroke(p.cr);

	cairo_scale(p.cr, zoom, zoom);
	cairo_translate(p.cr, - creal(top_left) + radius, - cimag(top_left) + radius);

	cairo_set_source_rgba(p.cr, 1, 1, 1, .7);
	cairo_set_line_width(p.cr, 2. * radius);
	cairo_set_line_cap(p.cr, CAIRO_LINE_CAP_ROUND);
	cairo_set_line_join(p.cr, CAIRO_LINE_JOIN_ROUND);
	cairo_move_to(p.cr, creal(hit->p), cimag(hit->p));
	for (int i = 1; i <= 128; ++i) {
		oshu_point pt = oshu_path_at(&hit->slider.path, (double) i / 128);
		cairo_line_to(p.cr, creal(pt), cimag(pt));
	}
	cairo_stroke(p.cr);

	cairo_set_source_rgba(p.cr, .1, .1, .1, 1);
	cairo_set_line_width(p.cr, 3);
	cairo_arc(p.cr, creal(hit->p), cimag(hit->p), radius - 4, 0, 2. * M_PI);
	cairo_stroke(p.cr);

	oshu_point end = oshu_path_at(&hit->slider.path, 1.);
	cairo_set_source_rgba(p.cr, .1, .1, .1, .5);
	cairo_set_line_width(p.cr, 1);
	for (int i = 1; i <= hit->slider.repeat; ++i) {
		cairo_arc(p.cr, creal(end), cimag(end), (radius - 4.) / i, 0, 2. * M_PI);
		cairo_stroke(p.cr);
	}

	hit->texture = calloc(1, sizeof(*hit->texture));
	assert (hit->texture != NULL);
	if (oshu_finish_painting(&p, &game->display, hit->texture) < 0) {
		free(hit->texture);
		hit->texture = NULL;
		return -1;
	}

	hit->texture->size = size;
	hit->texture->origin = hit->p - top_left + radius;
	return 0;
}

int osu_paint_resources(struct oshu_game *game)
{
	for (struct oshu_hit *hit = game->beatmap.hits; hit; hit = hit->next) {
		if (hit->type & OSHU_SLIDER_HIT)
			paint_slider(game, hit);
	}
	return paint_circle(game);
}

int osu_free_resources(struct oshu_game *game)
{
	for (struct oshu_hit *hit = game->beatmap.hits; hit; hit = hit->next) {
		if (hit->texture) {
			oshu_destroy_texture(hit->texture);
			free(hit->texture);
		}
	}
	oshu_destroy_texture(&game->osu.circle_texture);
	return 0;
}

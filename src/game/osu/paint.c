/**
 * \file game/osu/paint.h
 * \ingroup osu
 */

#include "game/game.h"
#include "graphics/paint.h"
#include "log.h"

#include <assert.h>

static double brighter(double v)
{
	v += .3;
	return v < 1. ? v : 1.;
}

static int paint_approach_circle(struct oshu_game *game) {
	double radius = game->beatmap.difficulty.circle_radius + game->beatmap.difficulty.approach_size;
	oshu_size size = (1. + I) * radius * 2.;
	double zoom = game->display.view.zoom;

	struct oshu_painter p;
	oshu_start_painting(size * zoom, &p);
	cairo_scale(p.cr, zoom, zoom);
	cairo_translate(p.cr, radius, radius);

	cairo_arc(p.cr, 0, 0, radius - 3, 0, 2. * M_PI);
	cairo_set_source_rgba(p.cr, 1., 1., 1., .5);
	cairo_set_line_width(p.cr, 4);
	cairo_stroke(p.cr);

	struct oshu_texture *texture = &game->osu.approach_circle;
	int rc = oshu_finish_painting(&p, &game->display, texture);
	texture->size = size;
	texture->origin = size / 2.;
	return rc;
}

static int paint_circle(struct oshu_game *game, struct oshu_color *color, struct oshu_texture *texture)
{
	double radius = game->beatmap.difficulty.circle_radius;
	oshu_size size = (1. + I) * radius * 2.;
	double zoom = game->display.view.zoom;

	struct oshu_painter p;
	oshu_start_painting(size * zoom, &p);
	cairo_scale(p.cr, zoom, zoom);
	cairo_translate(p.cr, radius, radius);

	cairo_arc(p.cr, 0, 0, radius - 4, 0, 2. * M_PI);
	cairo_set_source_rgb(p.cr, 1., 1., 1.);
	cairo_set_line_width(p.cr, 5);
	cairo_stroke_preserve(p.cr);

	cairo_pattern_t *pattern = cairo_pattern_create_radial(
		-radius, -radius, 0.,
		-radius, -radius, 2. * radius
	);
	cairo_pattern_add_color_stop_rgb(pattern, 0,
		brighter(color->red), brighter(color->green), brighter(color->blue));
	cairo_pattern_add_color_stop_rgb(pattern, 1, color->red, color->green, color->blue);
	cairo_set_source(p.cr, pattern);
	cairo_fill_preserve(p.cr);
	cairo_pattern_destroy(pattern);

	cairo_set_source_rgb(p.cr, 0., 0., 0.);
	cairo_set_line_width(p.cr, 3);
	cairo_stroke(p.cr);

	int rc = oshu_finish_painting(&p, &game->display, texture);
	texture->size = size;
	texture->origin = size / 2.;
	return rc;
}

static int paint_slider(struct oshu_game *game, struct oshu_hit *hit)
{
	assert (hit->type & OSHU_SLIDER_HIT);
	oshu_vector radius = game->beatmap.difficulty.circle_radius * (1. + I);
	oshu_point top_left, bottom_right;
	oshu_path_bounding_box(&hit->slider.path, &top_left, &bottom_right);
	oshu_size size = bottom_right - top_left + 2. * radius;
	double zoom = game->display.view.zoom;

	struct oshu_painter p;
	oshu_start_painting(size * zoom, &p);

	cairo_scale(p.cr, zoom, zoom);
	cairo_translate(p.cr, - creal(top_left) + radius, - cimag(top_left) + radius);

	/* Path. */
	cairo_set_line_cap(p.cr, CAIRO_LINE_CAP_ROUND);
	cairo_set_line_join(p.cr, CAIRO_LINE_JOIN_ROUND);
	cairo_move_to(p.cr, creal(hit->p), cimag(hit->p));
	for (int i = 1; i <= 128; ++i) {
		oshu_point pt = oshu_path_at(&hit->slider.path, (double) i / 128);
		cairo_line_to(p.cr, creal(pt), cimag(pt));
	}

	/* Slider body. */
	cairo_set_source_rgb(p.cr, 1., 1., 1.);
	cairo_set_line_width(p.cr, 2. * radius - 2);
	cairo_stroke_preserve(p.cr);

	cairo_set_source_rgb(p.cr, 0., 0., 0.);
	cairo_set_line_width(p.cr, 2. * radius - 4);
	cairo_stroke_preserve(p.cr);

	cairo_pattern_t *pattern = cairo_pattern_create_radial(
		creal(top_left), cimag(top_left), 0.,
		creal(top_left), cimag(top_left), cabs(size / 1.5)
	);
	cairo_pattern_add_color_stop_rgb(pattern, 0, 1., 1., 1.);
	cairo_pattern_add_color_stop_rgb(pattern, 0,
		brighter(hit->color->red), brighter(hit->color->green), brighter(hit->color->blue));
	cairo_pattern_add_color_stop_rgb(pattern, 1, hit->color->red, hit->color->green, hit->color->blue);
	cairo_set_source(p.cr, pattern);
	cairo_set_line_width(p.cr, 2. * radius - 8);
	cairo_stroke(p.cr);

	/* End point. */
	oshu_point end = oshu_path_at(&hit->slider.path, 1.);
	cairo_set_source_rgb(p.cr, 0., 0., 0.);
	cairo_set_line_width(p.cr, 1);
	for (int i = 1; i <= hit->slider.repeat; ++i) {
		double ratio = (double) i / hit->slider.repeat;
		cairo_arc(p.cr, creal(end), cimag(end), (radius - 4.) * ratio, 0, 2. * M_PI);
		cairo_stroke(p.cr);
	}

	/* Start point. */
	cairo_arc(p.cr, creal(hit->p), cimag(hit->p), radius - 4, 0, 2. * M_PI);
	cairo_set_source(p.cr, pattern);
	cairo_fill_preserve(p.cr);

	cairo_set_source_rgb(p.cr, 0., 0., 0.);
	cairo_set_line_width(p.cr, 2.5);
	cairo_stroke(p.cr);

	cairo_pattern_destroy(pattern);

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

static int paint_slider_ball(struct oshu_game *game) {
	double radius = game->beatmap.difficulty.slider_tolerance;
	oshu_size size = (1. + I) * radius * 2.;
	double zoom = game->display.view.zoom;

	struct oshu_painter p;
	oshu_start_painting(size * zoom, &p);
	cairo_scale(p.cr, zoom, zoom);
	cairo_translate(p.cr, radius, radius);

	/* tolerance */
	cairo_arc(p.cr, 0, 0, radius - 2, 0, 2. * M_PI);
	cairo_set_source_rgba(p.cr, 1., 1., 1., .5);
	cairo_set_line_width(p.cr, 3);
	cairo_stroke(p.cr);

	/* ball */
	cairo_arc(p.cr, 0, 0, game->beatmap.difficulty.circle_radius / 2, 0, 2. * M_PI);
	cairo_set_source_rgba(p.cr, 1., 1., 1., .8);
	cairo_fill_preserve(p.cr);
	cairo_set_source_rgba(p.cr, 0., 0., 0., .5);
	cairo_set_line_width(p.cr, 2);
	cairo_stroke(p.cr);

	struct oshu_texture *texture = &game->osu.slider_ball;
	int rc = oshu_finish_painting(&p, &game->display, texture);
	texture->size = size;
	texture->origin = size / 2.;
	return rc;
}

static int paint_good_mark(struct oshu_game *game)
{
	double radius = game->beatmap.difficulty.circle_radius / 3;
	oshu_size size = (1. + I) * radius * 2.;
	double zoom = game->display.view.zoom;

	struct oshu_painter p;
	oshu_start_painting(size * zoom, &p);
	cairo_scale(p.cr, zoom, zoom);
	cairo_translate(p.cr, radius, radius);

	cairo_arc(p.cr, 0, 0, radius - 3, 0, 2. * M_PI);
	cairo_set_source_rgba(p.cr, 0, .7, 0, 1.);
	cairo_set_line_width(p.cr, 2);
	cairo_stroke(p.cr);

	struct oshu_texture *texture = &game->osu.good_mark;
	int rc = oshu_finish_painting(&p, &game->display, texture);
	texture->size = size;
	texture->origin = size / 2.;
	return rc;
}

static int paint_bad_mark(struct oshu_game *game)
{
	double half = game->beatmap.difficulty.circle_radius / 4;
	oshu_size size = (1. + I) * (half + 4) * 2.;
	double zoom = game->display.view.zoom;

	struct oshu_painter p;
	oshu_start_painting(size * zoom, &p);
	cairo_scale(p.cr, zoom, zoom);
	cairo_translate(p.cr, half + 2, half + 2);

	cairo_set_source_rgba(p.cr, .8, 0, 0, 1.);
	cairo_set_line_width(p.cr, 2);

	cairo_move_to(p.cr, -half, -half);
	cairo_line_to(p.cr, half, half);
	cairo_stroke(p.cr);

	cairo_move_to(p.cr, half, -half);
	cairo_line_to(p.cr, -half, half);
	cairo_stroke(p.cr);

	struct oshu_texture *texture = &game->osu.bad_mark;
	int rc = oshu_finish_painting(&p, &game->display, texture);
	texture->size = size;
	texture->origin = size / 2.;
	return rc;
}

int osu_paint_resources(struct oshu_game *game)
{
	/* Circle hits. */
	assert (game->beatmap.color_count > 0);
	assert (game->beatmap.colors != NULL);
	game->osu.circles = calloc(game->beatmap.color_count, sizeof(*game->osu.circles));
	assert (game->osu.circles != NULL);
	struct oshu_color *color = game->beatmap.colors;
	for (int i = 0; i < game->beatmap.color_count; ++i) {
		oshu_log_verbose("painting circle for combo color #%d", i);
		assert (color->index == i);
		paint_circle(game, color, &game->osu.circles[i]);
		color = color->next;
	}

	/* Sliders. */
	for (struct oshu_hit *hit = game->beatmap.hits; hit; hit = hit->next) {
		if (hit->type & OSHU_SLIDER_HIT)
			paint_slider(game, hit);
	}

	paint_approach_circle(game);
	paint_slider_ball(game);
	paint_good_mark(game);
	paint_bad_mark(game);
	return 0;
}

int osu_free_resources(struct oshu_game *game)
{
	for (int i = 0; i < game->beatmap.color_count; ++i)
		oshu_destroy_texture(&game->osu.circles[i]);
	free(game->osu.circles);
	for (struct oshu_hit *hit = game->beatmap.hits; hit; hit = hit->next) {
		if (hit->texture) {
			oshu_destroy_texture(hit->texture);
			free(hit->texture);
		}
	}
	oshu_destroy_texture(&game->osu.approach_circle);
	oshu_destroy_texture(&game->osu.slider_ball);
	oshu_destroy_texture(&game->osu.good_mark);
	oshu_destroy_texture(&game->osu.bad_mark);
	return 0;
}
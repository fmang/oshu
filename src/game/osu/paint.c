/**
 * \file game/osu/paint.c
 * \ingroup osu
 */

#include "game/game.h"
#include "graphics/paint.h"
#include "log.h"

#include <assert.h>
#include <SDL2/SDL_timer.h>

static double brighter(double v)
{
	v += .3;
	return v < 1. ? v : 1.;
}

static int paint_cursor(struct oshu_game *game) {
	double radius = 12;
	oshu_size size = (1. + I) * radius * 2.;
	double zoom = game->display.view.zoom;

	struct oshu_painter p;
	oshu_start_painting(size * zoom, &p);
	cairo_scale(p.cr, zoom, zoom);
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

	struct oshu_texture *texture = &game->osu.cursor;
	int rc = oshu_finish_painting(&p, &game->display, texture);
	texture->size = size;
	texture->origin = size / 2.;
	return rc;
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
	cairo_set_source_rgba(p.cr, 1., 1., 1., .6);
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
	cairo_set_operator(p.cr, CAIRO_OPERATOR_SOURCE);
	double opacity = 0.6;

	cairo_arc(p.cr, 0, 0, radius - 4, 0, 2. * M_PI);
	cairo_set_source_rgba(p.cr, 1., 1., 1., opacity);
	cairo_set_line_width(p.cr, 5);
	cairo_stroke_preserve(p.cr);

	cairo_pattern_t *pattern = cairo_pattern_create_radial(
		-radius, -radius, 0.,
		-radius, -radius, 2. * radius
	);
	cairo_pattern_add_color_stop_rgba(pattern, 0,
		brighter(color->red), brighter(color->green), brighter(color->blue), opacity);
	cairo_pattern_add_color_stop_rgba(pattern, 1, color->red, color->green, color->blue, opacity);
	cairo_set_source(p.cr, pattern);
	cairo_fill_preserve(p.cr);
	cairo_pattern_destroy(pattern);

	cairo_set_source_rgba(p.cr, 0., 0., 0., opacity);
	cairo_set_line_width(p.cr, 3);
	cairo_stroke(p.cr);

	int rc = oshu_finish_painting(&p, &game->display, texture);
	texture->size = size;
	texture->origin = size / 2.;
	return rc;
}

static void build_path(cairo_t *cr, struct oshu_slider *slider)
{
	if (slider->path.type == OSHU_LINEAR_PATH) {
		cairo_move_to(cr, creal(slider->path.line.start), cimag(slider->path.line.start));
		cairo_line_to(cr, creal(slider->path.line.end), cimag(slider->path.line.end));
	} else if (slider->path.type == OSHU_PERFECT_PATH) {
		struct oshu_arc *arc = &slider->path.arc;
		if (arc->start_angle < arc->end_angle)
			cairo_arc(cr, creal(arc->center), cimag(arc->center), arc->radius, arc->start_angle, arc->end_angle);
		else
			cairo_arc_negative(cr, creal(arc->center), cimag(arc->center), arc->radius, arc->start_angle, arc->end_angle);
	} else {
		oshu_point start = oshu_path_at(&slider->path, 0);
		cairo_move_to(cr, creal(start), cimag(start));
		int resolution = slider->length / 5. + 5;
		for (int i = 1; i <= resolution; ++i) {
			oshu_point pt = oshu_path_at(&slider->path, (double) i / resolution);
			cairo_line_to(cr, creal(pt), cimag(pt));
		}
	}
}

int osu_paint_slider(struct oshu_game *game, struct oshu_hit *hit)
{
	int start = SDL_GetTicks();
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
	cairo_set_operator(p.cr, CAIRO_OPERATOR_SOURCE);
	double opacity = 0.6;

	/* Path. */
	cairo_set_line_cap(p.cr, CAIRO_LINE_CAP_ROUND);
	cairo_set_line_join(p.cr, CAIRO_LINE_JOIN_ROUND);
	build_path(p.cr, &hit->slider);

	/* Slider body. */
	cairo_set_source_rgba(p.cr, 1., 1., 1., opacity);
	cairo_set_line_width(p.cr, 2. * radius - 2);
	cairo_stroke_preserve(p.cr);

	cairo_set_source_rgba(p.cr, 0., 0., 0., opacity);
	cairo_set_line_width(p.cr, 2. * radius - 4);
	cairo_stroke_preserve(p.cr);

	cairo_pattern_t *pattern = cairo_pattern_create_radial(
		creal(top_left), cimag(top_left), 0.,
		creal(top_left), cimag(top_left), cabs(size / 1.5)
	);
	cairo_pattern_add_color_stop_rgba(pattern, 0,
		brighter(hit->color->red), brighter(hit->color->green), brighter(hit->color->blue), opacity);
	cairo_pattern_add_color_stop_rgba(pattern, 1, hit->color->red, hit->color->green, hit->color->blue, opacity);
	cairo_set_source(p.cr, pattern);
	cairo_set_line_width(p.cr, 2. * radius - 8);
	cairo_stroke(p.cr);

	/* End point. */
	oshu_point end = oshu_path_at(&hit->slider.path, 1.);
	cairo_set_source_rgba(p.cr, 0., 0., 0., opacity);
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

	cairo_set_source_rgba(p.cr, 0., 0., 0., opacity);
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
	oshu_log_verbose("slider drawn in %.3f seconds", (SDL_GetTicks() - start) / 1000.);
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
	cairo_arc(p.cr, 0, 0, game->beatmap.difficulty.circle_radius / 2.2, 0, 2. * M_PI);
	cairo_set_source_rgba(p.cr, 1., 1., 1., .5);
	cairo_fill(p.cr);

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
	cairo_set_source_rgba(p.cr, 0, .8, 0, .4);
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
	double half = game->beatmap.difficulty.circle_radius / 4.2;
	oshu_size size = (1. + I) * (half + 4) * 2.;
	double zoom = game->display.view.zoom;

	struct oshu_painter p;
	oshu_start_painting(size * zoom, &p);
	cairo_scale(p.cr, zoom, zoom);
	cairo_translate(p.cr, half + 2, half + 2);

	cairo_set_source_rgba(p.cr, .9, 0, 0, .4);
	cairo_set_operator(p.cr, CAIRO_OPERATOR_SOURCE);
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

static int paint_skip_mark(struct oshu_game *game)
{
	double radius = game->beatmap.difficulty.circle_radius / 3.5;
	oshu_size size = (1. + I) * (radius + 4) * 2.;
	double zoom = game->display.view.zoom;

	struct oshu_painter p;
	oshu_start_painting(size * zoom, &p);
	cairo_scale(p.cr, zoom, zoom);
	cairo_translate(p.cr, radius + 2, radius + 2);

	cairo_set_source_rgba(p.cr, 0, 0, .9, .4);
	cairo_set_line_width(p.cr, 1);

	oshu_vector a = radius;
	oshu_vector b = a * cexp(2. * M_PI / 3. * I);
	oshu_vector c = b * cexp(2. * M_PI / 3. * I);

	cairo_move_to(p.cr, creal(a), cimag(a));
	cairo_line_to(p.cr, creal(b), cimag(b));
	cairo_line_to(p.cr, creal(c), cimag(c));
	cairo_close_path(p.cr);
	cairo_stroke(p.cr);

	struct oshu_texture *texture = &game->osu.skip_mark;
	int rc = oshu_finish_painting(&p, &game->display, texture);
	texture->size = size;
	texture->origin = size / 2.;
	return rc;
}

/**
 * \todo
 * Handle errors.
 */
int osu_paint_resources(struct oshu_game *game)
{
	int start = SDL_GetTicks();
	oshu_log_debug("painting the textures");

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

	paint_cursor(game);
	paint_approach_circle(game);
	paint_slider_ball(game);
	paint_good_mark(game);
	paint_bad_mark(game);
	paint_skip_mark(game);

	int end = SDL_GetTicks();
	oshu_log_debug("done generating the common textures in %.3f seconds", (end - start) / 1000.);
	return 0;
}

void osu_free_resources(struct oshu_game *game)
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
	oshu_destroy_texture(&game->osu.cursor);
	oshu_destroy_texture(&game->osu.approach_circle);
	oshu_destroy_texture(&game->osu.slider_ball);
	oshu_destroy_texture(&game->osu.good_mark);
	oshu_destroy_texture(&game->osu.bad_mark);
	oshu_destroy_texture(&game->osu.skip_mark);
}

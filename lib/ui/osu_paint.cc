/**
 * \file lib/ui/osu_paint.cc
 * \ingroup ui
 */

#include "ui/osu.h"

#include "core/log.h"
#include "game/osu.h"
#include "video/paint.h"

#include <assert.h>
#include <SDL2/SDL_timer.h>

static double brighter(double v)
{
	v += .3;
	return v < 1. ? v : 1.;
}

static int paint_approach_circle(oshu::ui::osu &view)
{
	oshu::game::base *game = &view.game;
	double radius = game->beatmap.difficulty.circle_radius + game->beatmap.difficulty.approach_size;
	oshu_size size = oshu_size(radius * 2., radius * 2.);

	struct oshu_painter p;
	oshu_start_painting(view.display, size, &p);
	cairo_translate(p.cr, radius, radius);

	cairo_arc(p.cr, 0, 0, radius - 3, 0, 2. * M_PI);
	cairo_set_source_rgba(p.cr, 1., 1., 1., .6);
	cairo_set_line_width(p.cr, 4);
	cairo_stroke(p.cr);

	struct oshu_texture *texture = &view.approach_circle;
	int rc = oshu_finish_painting(&p, texture);
	texture->origin = size / 2.;
	return rc;
}

static int paint_circle(oshu::ui::osu &view, struct oshu_color *color, struct oshu_texture *texture)
{
	oshu::game::base *game = &view.game;
	double radius = game->beatmap.difficulty.circle_radius;
	oshu_size size = oshu_size(radius * 2., radius * 2.);

	struct oshu_painter p;
	oshu_start_painting(view.display, size, &p);
	cairo_translate(p.cr, radius, radius);
	cairo_set_operator(p.cr, CAIRO_OPERATOR_SOURCE);
	double opacity = 0.7;

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

	int rc = oshu_finish_painting(&p, texture);
	texture->origin = size / 2.;
	return rc;
}

static void build_path(cairo_t *cr, struct oshu_slider *slider)
{
	if (slider->path.type == OSHU_LINEAR_PATH) {
		cairo_move_to(cr, std::real(slider->path.line.start), std::imag(slider->path.line.start));
		cairo_line_to(cr, std::real(slider->path.line.end), std::imag(slider->path.line.end));
	} else if (slider->path.type == OSHU_PERFECT_PATH) {
		struct oshu_arc *arc = &slider->path.arc;
		if (arc->start_angle < arc->end_angle)
			cairo_arc(cr, std::real(arc->center), std::imag(arc->center), arc->radius, arc->start_angle, arc->end_angle);
		else
			cairo_arc_negative(cr, std::real(arc->center), std::imag(arc->center), arc->radius, arc->start_angle, arc->end_angle);
	} else {
		oshu_point start = oshu_path_at(&slider->path, 0);
		cairo_move_to(cr, std::real(start), std::imag(start));
		int resolution = slider->length / 5. + 5;
		for (int i = 1; i <= resolution; ++i) {
			oshu_point pt = oshu_path_at(&slider->path, (double) i / resolution);
			cairo_line_to(cr, std::real(pt), std::imag(pt));
		}
	}
}

/**
 * \todo
 * Today, the number of repeat is shown by drawing concentric circles at the
 * end of the slider. However, these circles are drawn statically, while we'd
 * like them to disappear one after another as the slider repeats.
 *
 * \todo
 * Paint the slider ticks. Preferably updating the ticks every time the slider
 * repeats. Also, clear the ticks as the slider rolls over them.
 */
int osu_paint_slider(oshu::ui::osu &view, struct oshu_hit *hit)
{
	oshu::game::base *game = &view.game;
	int start = SDL_GetTicks();
	assert (hit->type & OSHU_SLIDER_HIT);
	double radius = game->beatmap.difficulty.circle_radius;
	oshu_point top_left, bottom_right;
	oshu_path_bounding_box(&hit->slider.path, &top_left, &bottom_right);
	oshu_size size = bottom_right - top_left + oshu_vector{2, 2} * radius;

	struct oshu_painter p;
	oshu_start_painting(view.display, size, &p);

	cairo_translate(p.cr, - std::real(top_left) + radius, - std::imag(top_left) + radius);
	cairo_set_operator(p.cr, CAIRO_OPERATOR_SOURCE);
	double opacity = 0.7;

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
		std::real(top_left), std::imag(top_left), 0.,
		std::real(top_left), std::imag(top_left), std::abs(size / 1.5)
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
		cairo_arc(p.cr, std::real(end), std::imag(end), (radius - 4.) * ratio, 0, 2. * M_PI);
		cairo_stroke(p.cr);
	}

	/* Start point. */
	cairo_arc(p.cr, std::real(hit->p), std::imag(hit->p), radius - 4, 0, 2. * M_PI);
	cairo_set_source(p.cr, pattern);
	cairo_fill_preserve(p.cr);

	cairo_set_source_rgba(p.cr, 0., 0., 0., opacity);
	cairo_set_line_width(p.cr, 2.5);
	cairo_stroke(p.cr);

	cairo_pattern_destroy(pattern);

	hit->texture = (oshu_texture*) calloc(1, sizeof(*hit->texture));
	assert (hit->texture != NULL);
	if (oshu_finish_painting(&p, hit->texture) < 0) {
		free(hit->texture);
		hit->texture = NULL;
		return -1;
	}

	hit->texture->origin = hit->p - top_left + oshu_vector{1, 1} * radius;
	oshu_log_verbose("slider drawn in %.3f seconds", (SDL_GetTicks() - start) / 1000.);
	return 0;
}

/**
 * \todo
 * It looks like cairo_fill with a pattern triggers jumps depending on
 * uninitialised values, which propagates.
 */
static int paint_slider_ball(oshu::ui::osu &view) {
	oshu::game::base *game = &view.game;
	double radius = game->beatmap.difficulty.slider_tolerance;
	oshu_size size = oshu_size{1, 1} * radius * 2.;

	struct oshu_painter p;
	oshu_start_painting(view.display, size, &p);
	cairo_translate(p.cr, radius, radius);

	/* tolerance */
	cairo_arc(p.cr, 0, 0, radius - 2, 0, 2. * M_PI);
	cairo_set_source_rgba(p.cr, 1., 1., 1., .5);
	cairo_set_line_width(p.cr, 3);
	cairo_stroke(p.cr);

	/* ball */
	double ball_radius = game->beatmap.difficulty.circle_radius / 1.2;
	cairo_pattern_t *pattern = cairo_pattern_create_radial(
		0, 0, 0,
		0, 0, ball_radius
	);
	cairo_pattern_add_color_stop_rgba(pattern, 0.0, 1, 1, 1, .7);
	cairo_pattern_add_color_stop_rgba(pattern, 0.4, 1, 1, 1, .6);
	cairo_pattern_add_color_stop_rgba(pattern, 0.6, 1, 1, 1, .2);
	cairo_pattern_add_color_stop_rgba(pattern, 1.0, 1, 1, 1, 0);
	cairo_arc(p.cr, 0, 0, ball_radius, 0, 2. * M_PI);
	cairo_set_source(p.cr, pattern);
	cairo_fill(p.cr);
	cairo_pattern_destroy(pattern);

	struct oshu_texture *texture = &view.slider_ball;
	int rc = oshu_finish_painting(&p, texture);
	texture->origin = size / 2.;
	return rc;
}

static int paint_good_mark(oshu::ui::osu &view, int offset, struct oshu_texture *texture)
{
	oshu::game::base *game = &view.game;
	double radius = game->beatmap.difficulty.circle_radius / 3.5;
	oshu_size size = oshu_size{1, 1} * radius * 2.;

	struct oshu_painter p;
	oshu_start_painting(view.display, size, &p);
	cairo_translate(p.cr, radius, radius);

	if (offset == 0) {
		/* good */
		cairo_set_source_rgba(p.cr, 0, .8, 0, .4);
		cairo_arc(p.cr, 0, 0, radius - 3, 0, 2. * M_PI);
	} else if (offset < 0) {
		/* early */
		cairo_set_source_rgba(p.cr, .8, .8, 0, .4);
		cairo_arc(p.cr, 0, 0, radius - 3, M_PI / 2 , 3 * M_PI / 2);
	} else if (offset > 0) {
		/* late */
		cairo_set_source_rgba(p.cr, .8, .8, 0, .4);
		cairo_arc(p.cr, 0, 0, radius - 3, - M_PI / 2 , M_PI / 2);
	}
	cairo_set_line_width(p.cr, 2);
	cairo_stroke(p.cr);

	int rc = oshu_finish_painting(&p, texture);
	texture->origin = size / 2.;
	return rc;
}

static int paint_bad_mark(oshu::ui::osu &view)
{
	oshu::game::base *game = &view.game;
	double half = game->beatmap.difficulty.circle_radius / 4.7;
	oshu_size size = oshu_size{1, 1} * (half + 2) * 2.;

	struct oshu_painter p;
	oshu_start_painting(view.display, size, &p);
	cairo_translate(p.cr, half + 2, half + 2);

	cairo_set_source_rgba(p.cr, .9, 0, 0, .4);
	cairo_set_line_width(p.cr, 2);

	cairo_move_to(p.cr, -half, -half);
	cairo_line_to(p.cr, half, half);

	cairo_move_to(p.cr, half, -half);
	cairo_line_to(p.cr, -half, half);

	cairo_stroke(p.cr);

	struct oshu_texture *texture = &view.bad_mark;
	int rc = oshu_finish_painting(&p, texture);
	texture->origin = size / 2.;
	return rc;
}

static int paint_skip_mark(oshu::ui::osu &view)
{
	oshu::game::base *game = &view.game;
	double radius = game->beatmap.difficulty.circle_radius / 4.7;
	oshu_size size = oshu_size{1, 1} * (radius + 2) * 2.;

	struct oshu_painter p;
	oshu_start_painting(view.display, size, &p);
	cairo_translate(p.cr, radius + 2, radius + 2);

	cairo_set_source_rgba(p.cr, .3, .3, 1, .6);
	cairo_set_line_width(p.cr, 1);

	cairo_move_to(p.cr, 0, radius);
	cairo_line_to(p.cr, radius, 0);
	cairo_line_to(p.cr, 0, -radius);

	cairo_move_to(p.cr, -radius, radius);
	cairo_line_to(p.cr, 0, 0);
	cairo_line_to(p.cr, -radius, -radius);

	cairo_stroke(p.cr);

	struct oshu_texture *texture = &view.skip_mark;
	int rc = oshu_finish_painting(&p, texture);
	texture->origin = size / 2.;
	return rc;
}

static int paint_connector(oshu::ui::osu &view)
{
	oshu::game::base *game = &view.game;
	double radius = 3;
	oshu_size size = oshu_size{1, 1} * radius * 2.;

	struct oshu_painter p;
	oshu_start_painting(view.display, size, &p);
	cairo_translate(p.cr, radius, radius);

	cairo_set_source_rgba(p.cr, 1, 1, 1, .5);
	cairo_arc(p.cr, 0, 0, radius - 1, 0, 2. * M_PI);
	cairo_fill(p.cr);

	struct oshu_texture *texture = &view.connector;
	int rc = oshu_finish_painting(&p, texture);
	texture->origin = size / 2.;
	return rc;
}

/**
 * \todo
 * Handle errors.
 */
int osu_paint_resources(oshu::ui::osu &view)
{
	oshu::game::base *game = &view.game;
	int start = SDL_GetTicks();
	oshu_log_debug("painting the textures");

	/* Circle hits. */
	assert (game->beatmap.color_count > 0);
	assert (game->beatmap.colors != NULL);
	view.circles = (oshu_texture*) calloc(game->beatmap.color_count, sizeof(*view.circles));
	assert (view.circles != NULL);
	struct oshu_color *color = game->beatmap.colors;
	for (int i = 0; i < game->beatmap.color_count; ++i) {
		oshu_log_verbose("painting circle for combo color #%d", i);
		assert (color->index == i);
		paint_circle(view, color, &view.circles[i]);
		color = color->next;
	}

	paint_approach_circle(view);
	paint_slider_ball(view);
	paint_good_mark(view, -1, &view.early_mark);
	paint_good_mark(view, 0, &view.good_mark);
	paint_good_mark(view, 1, &view.late_mark);
	paint_bad_mark(view);
	paint_skip_mark(view);
	paint_connector(view);

	int end = SDL_GetTicks();
	oshu_log_debug("done generating the common textures in %.3f seconds", (end - start) / 1000.);
	return 0;
}

void osu_free_resources(oshu::ui::osu &view)
{
	oshu::game::base *game = &view.game;
	if (view.circles) {
		for (int i = 0; i < game->beatmap.color_count; ++i)
			oshu_destroy_texture(&view.circles[i]);
		free(view.circles);
	}
	for (struct oshu_hit *hit = game->beatmap.hits; hit; hit = hit->next) {
		if (hit->texture) {
			oshu_destroy_texture(hit->texture);
			free(hit->texture);
			hit->texture = NULL;
		}
	}
	oshu_destroy_texture(&view.approach_circle);
	oshu_destroy_texture(&view.slider_ball);
	oshu_destroy_texture(&view.good_mark);
	oshu_destroy_texture(&view.early_mark);
	oshu_destroy_texture(&view.late_mark);
	oshu_destroy_texture(&view.bad_mark);
	oshu_destroy_texture(&view.skip_mark);
	oshu_destroy_texture(&view.connector);
}

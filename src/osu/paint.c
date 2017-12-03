/**
 * \file osu/paint.c
 * \ingroup osu
 */

#include "../config.h"

#include "game/game.h"
#include "graphics/paint.h"
#include "log.h"

#include <assert.h>
#include <SDL2/SDL_timer.h>
#include <pango/pangocairo.h>

static double brighter(double v)
{
	v += .3;
	return v < 1. ? v : 1.;
}

static int paint_cursor(struct oshu_game *game) {
	double radius = 14;
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

	struct oshu_texture *texture = &game->osu.cursor;
	int rc = oshu_finish_painting(&p, texture);
	texture->origin = size / 2.;
	return rc;
}

static int paint_approach_circle(struct oshu_game *game)
{
	double radius = game->beatmap.difficulty.circle_radius + game->beatmap.difficulty.approach_size;
	oshu_size size = (1. + I) * radius * 2.;

	struct oshu_painter p;
	oshu_start_painting(&game->display, size, &p);
	cairo_translate(p.cr, radius, radius);

	cairo_arc(p.cr, 0, 0, radius - 3, 0, 2. * M_PI);
	cairo_set_source_rgba(p.cr, 1., 1., 1., .6);
	cairo_set_line_width(p.cr, 4);
	cairo_stroke(p.cr);

	struct oshu_texture *texture = &game->osu.approach_circle;
	int rc = oshu_finish_painting(&p, texture);
	texture->origin = size / 2.;
	return rc;
}

static int paint_circle(struct oshu_game *game, struct oshu_color *color, struct oshu_texture *texture)
{
	double radius = game->beatmap.difficulty.circle_radius;
	oshu_size size = (1. + I) * radius * 2.;

	struct oshu_painter p;
	oshu_start_painting(&game->display, size, &p);
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
int osu_paint_slider(struct oshu_game *game, struct oshu_hit *hit)
{
	int start = SDL_GetTicks();
	assert (hit->type & OSHU_SLIDER_HIT);
	oshu_vector radius = game->beatmap.difficulty.circle_radius * (1. + I);
	oshu_point top_left, bottom_right;
	oshu_path_bounding_box(&hit->slider.path, &top_left, &bottom_right);
	oshu_size size = bottom_right - top_left + 2. * radius;

	struct oshu_painter p;
	oshu_start_painting(&game->display, size, &p);

	cairo_translate(p.cr, - creal(top_left) + radius, - cimag(top_left) + radius);
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
	if (oshu_finish_painting(&p, hit->texture) < 0) {
		free(hit->texture);
		hit->texture = NULL;
		return -1;
	}

	hit->texture->origin = hit->p - top_left + radius;
	oshu_log_verbose("slider drawn in %.3f seconds", (SDL_GetTicks() - start) / 1000.);
	return 0;
}

static int paint_slider_ball(struct oshu_game *game) {
	double radius = game->beatmap.difficulty.slider_tolerance;
	oshu_size size = (1. + I) * radius * 2.;

	struct oshu_painter p;
	oshu_start_painting(&game->display, size, &p);
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

	struct oshu_texture *texture = &game->osu.slider_ball;
	int rc = oshu_finish_painting(&p, texture);
	texture->origin = size / 2.;
	return rc;
}

static int paint_good_mark(struct oshu_game *game, int offset, struct oshu_texture *texture)
{
	double radius = game->beatmap.difficulty.circle_radius / 3.5;
	oshu_size size = (1. + I) * radius * 2.;

	struct oshu_painter p;
	oshu_start_painting(&game->display, size, &p);
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

static int paint_bad_mark(struct oshu_game *game)
{
	double half = game->beatmap.difficulty.circle_radius / 4.7;
	oshu_size size = (1. + I) * (half + 2) * 2.;

	struct oshu_painter p;
	oshu_start_painting(&game->display, size, &p);
	cairo_translate(p.cr, half + 2, half + 2);

	cairo_set_source_rgba(p.cr, .9, 0, 0, .4);
	cairo_set_line_width(p.cr, 2);

	cairo_move_to(p.cr, -half, -half);
	cairo_line_to(p.cr, half, half);

	cairo_move_to(p.cr, half, -half);
	cairo_line_to(p.cr, -half, half);

	cairo_stroke(p.cr);

	struct oshu_texture *texture = &game->osu.bad_mark;
	int rc = oshu_finish_painting(&p, texture);
	texture->origin = size / 2.;
	return rc;
}

static int paint_skip_mark(struct oshu_game *game)
{
	double radius = game->beatmap.difficulty.circle_radius / 4.7;
	oshu_size size = (1. + I) * (radius + 2) * 2.;

	struct oshu_painter p;
	oshu_start_painting(&game->display, size, &p);
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

	struct oshu_texture *texture = &game->osu.skip_mark;
	int rc = oshu_finish_painting(&p, texture);
	texture->origin = size / 2.;
	return rc;
}

static int paint_connector(struct oshu_game *game)
{
	double radius = 3;
	oshu_size size = (1. + I) * radius * 2.;

	struct oshu_painter p;
	oshu_start_painting(&game->display, size, &p);
	cairo_translate(p.cr, radius, radius);

	cairo_set_source_rgba(p.cr, 1, 1, 1, .5);
	cairo_arc(p.cr, 0, 0, radius - 1, 0, 2. * M_PI);
	cairo_fill(p.cr);

	struct oshu_texture *texture = &game->osu.connector;
	int rc = oshu_finish_painting(&p, texture);
	texture->origin = size / 2.;
	return rc;
}

static const double padding = 10;

static PangoLayout* setup_layout(struct oshu_painter *p)
{
	cairo_set_operator(p->cr, CAIRO_OPERATOR_SOURCE);

	PangoLayout *layout = pango_cairo_create_layout(p->cr);
	pango_layout_set_width(layout, PANGO_SCALE * (creal(p->size) - 2. * padding));
	pango_layout_set_ellipsize(layout, PANGO_ELLIPSIZE_END);
	pango_layout_set_spacing(layout, 5 * PANGO_SCALE);

	PangoFontDescription *desc = pango_font_description_from_string("Sans Bold 12");
	pango_layout_set_font_description(layout, desc);
	pango_font_description_free(desc);

	return layout;
}

static int paint_metadata(struct oshu_game *game, int unicode)
{
	oshu_size size = 640 + 60 * I;
	struct oshu_painter p;
	oshu_start_painting(&game->display, size, &p);
	PangoLayout *layout = setup_layout(&p);

	struct oshu_metadata *meta = &game->beatmap.metadata;
	const char *title = unicode ? meta->title_unicode : meta->title;
	const char *artist = unicode ? meta->artist_unicode : meta->artist;
	char *text;
	asprintf(&text, "%s\n%s", title, artist);
	assert (text != NULL);

	int width, height;
	pango_layout_set_text(layout, text, -1);
	pango_layout_get_size(layout, &width, &height);
	cairo_set_source_rgba(p.cr, 1, 1, 1, 1);
	cairo_move_to(p.cr, padding, (cimag(size) - height / PANGO_SCALE) / 2.);
	pango_cairo_show_layout(p.cr, layout);

	g_object_unref(layout);
	free(text);

	struct oshu_texture *texture = unicode ? &game->osu.metadata_unicode : &game->osu.metadata;
	int rc = oshu_finish_painting(&p, texture);
	texture->origin = 0;
	return rc;
}

static int paint_stars(struct oshu_game *game)
{
	oshu_size size = 360 + 60 * I;
	struct oshu_painter p;
	oshu_start_painting(&game->display, size, &p);
	PangoLayout *layout = setup_layout(&p);

	char *sky = " ★ ★ ★ ★ ★ ★ ★ ★ ★ ★";
	int stars = game->beatmap.difficulty.overall_difficulty;
	assert (stars >= 0);
	assert (stars <= 10);
	int star_length = strlen(sky) / 10;
	char *difficulty = strndup(sky, star_length * stars);

	struct oshu_metadata *meta = &game->beatmap.metadata;
	const char *version = meta->version;
	assert (version != NULL);
	char *text;
	asprintf(&text, "%s\n%s", version, difficulty);
	assert (text != NULL);

	int width, height;
	pango_layout_set_text(layout, text, -1);
	pango_layout_set_alignment(layout, PANGO_ALIGN_RIGHT);
	pango_layout_get_size(layout, &width, &height);
	cairo_set_source_rgba(p.cr, 1, 1, 1, .5);
	cairo_move_to(p.cr, padding, (cimag(size) - height / PANGO_SCALE) / 2.);
	pango_cairo_show_layout(p.cr, layout);

	g_object_unref(layout);
	free(text);
	free(difficulty);

	struct oshu_texture *texture = &game->osu.stars;
	int rc = oshu_finish_painting(&p, texture);
	texture->origin = creal(size);
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
	paint_good_mark(game, -1, &game->osu.early_mark);
	paint_good_mark(game, 0, &game->osu.good_mark);
	paint_good_mark(game, 1, &game->osu.late_mark);
	paint_bad_mark(game);
	paint_skip_mark(game);
	paint_connector(game);

	struct oshu_metadata *meta = &game->beatmap.metadata;
	int title_difference = meta->title && meta->title_unicode && strcmp(meta->title, meta->title_unicode);
	int artist_difference = meta->artist && meta->artist_unicode && strcmp(meta->artist, meta->artist_unicode);
	paint_metadata(game, 0);
	if (title_difference || artist_difference)
		paint_metadata(game, 1);

	paint_stars(game);

	int end = SDL_GetTicks();
	oshu_log_debug("done generating the common textures in %.3f seconds", (end - start) / 1000.);
	return 0;
}

void osu_free_resources(struct oshu_game *game)
{
	if (game->osu.circles) {
		for (int i = 0; i < game->beatmap.color_count; ++i)
			oshu_destroy_texture(&game->osu.circles[i]);
		free(game->osu.circles);
	}
	for (struct oshu_hit *hit = game->beatmap.hits; hit; hit = hit->next) {
		if (hit->texture) {
			oshu_destroy_texture(hit->texture);
			free(hit->texture);
			hit->texture = NULL;
		}
	}
	oshu_destroy_texture(&game->osu.cursor);
	oshu_destroy_texture(&game->osu.approach_circle);
	oshu_destroy_texture(&game->osu.slider_ball);
	oshu_destroy_texture(&game->osu.good_mark);
	oshu_destroy_texture(&game->osu.early_mark);
	oshu_destroy_texture(&game->osu.late_mark);
	oshu_destroy_texture(&game->osu.bad_mark);
	oshu_destroy_texture(&game->osu.skip_mark);
	oshu_destroy_texture(&game->osu.connector);
	oshu_destroy_texture(&game->osu.metadata);
	oshu_destroy_texture(&game->osu.metadata_unicode);
	oshu_destroy_texture(&game->osu.stars);
}

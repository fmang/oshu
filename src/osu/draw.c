/**
 * \file osu/draw.c
 * \ingroup osu
 *
 * \brief
 * Drawing routines specific to the osu!standard game mode.
 */

#include "game/game.h"
#include "graphics/texture.h"

#include <assert.h>

static void draw_hint(struct oshu_game *game, struct oshu_hit *hit)
{
	double now = game->clock.now;
	if (hit->time > now && hit->state == OSHU_INITIAL_HIT) {
		SDL_SetRenderDrawColor(game->display.renderer, 255, 128, 64, 255);
		double ratio = (double) (hit->time - now) / game->beatmap.difficulty.approach_time;
		double base_radius = game->beatmap.difficulty.circle_radius;
		double radius = base_radius + ratio * game->beatmap.difficulty.approach_size;
		oshu_draw_scaled_texture(
			&game->display, &game->osu.approach_circle, hit->p,
			2. * radius / creal(game->osu.approach_circle.size)
		);
	}
}

static void draw_hit_mark(struct oshu_game *game, struct oshu_hit *hit)
{
	if (hit->state == OSHU_GOOD_HIT) {
		double leniency = game->beatmap.difficulty.leniency;
		struct oshu_texture *mark = &game->osu.good_mark;
		if (hit->offset < - leniency / 2)
			mark = &game->osu.early_mark;
		else if (hit->offset > leniency / 2)
			mark = &game->osu.late_mark;
		oshu_draw_texture(&game->display, mark, oshu_end_point(hit));
	} else if (hit->state == OSHU_MISSED_HIT) {
		oshu_draw_texture(&game->display, &game->osu.bad_mark, oshu_end_point(hit));
	} else if (hit->state == OSHU_SKIPPED_HIT) {
		oshu_draw_texture(&game->display, &game->osu.skip_mark, oshu_end_point(hit));
	}
}

static void draw_hit_circle(struct oshu_game *game, struct oshu_hit *hit)
{
	struct oshu_display *display = &game->display;
	if (hit->state == OSHU_INITIAL_HIT) {
		assert (hit->color != NULL);
		oshu_draw_texture(display, &game->osu.circles[hit->color->index], hit->p);
		draw_hint(game, hit);
	} else {
		draw_hit_mark(game, hit);
	}
}

static void draw_slider(struct oshu_game *game, struct oshu_hit *hit)
{
	struct oshu_display *display = &game->display;
	double now = game->clock.now;
	if (hit->state == OSHU_INITIAL_HIT || hit->state == OSHU_SLIDING_HIT) {
		if (!hit->texture) {
			osu_paint_slider(game, hit);
			assert (hit->texture != NULL);
		}
		oshu_draw_texture(&game->display, hit->texture, hit->p);
		draw_hint(game, hit);
		/* ball */
		double t = (now - hit->time) / hit->slider.duration;
		if (hit->state == OSHU_SLIDING_HIT) {
			oshu_point ball = oshu_path_at(&hit->slider.path, t < 0 ? 0 : t);
			oshu_draw_texture(display, &game->osu.slider_ball, ball);
		}
	} else {
		draw_hit_mark(game, hit);
	}
}

static void draw_hit(struct oshu_game *game, struct oshu_hit *hit)
{
	if (hit->type & OSHU_SLIDER_HIT)
		draw_slider(game, hit);
	else if (hit->type & OSHU_CIRCLE_HIT)
		draw_hit_circle(game, hit);
}

/**
 * Connect two hits with a dotted line.
 *
 * ( ) · · · · ( )
 *
 * First, compute the visual distance between two hits, which is the distance
 * between the center, minus the two radii. Then split this interval in steps
 * of 15 pixels. Because we need an integral number of steps, floor it.
 *
 * The flooring would cause a extra padding after the last point, so we need to
 * recalibrate the interval by dividing the distance by the number of steps.
 *
 * Now we have our steps:
 *
 * ( )   |   |   |   |   ( )
 *
  However, for some reason, this yields an excessive visual margin before the
  first point and after the last point. To remedy this, the dots are put in the
  middle of the steps, instead of between.
 *
 * ( ) · | · | · | · | · ( )
 *
 * Voilà!
 *
 */
static void connect_hits(struct oshu_game *game, struct oshu_hit *a, struct oshu_hit *b)
{
	if (a->state != OSHU_INITIAL_HIT && a->state != OSHU_SLIDING_HIT)
		return;
	oshu_point a_end = oshu_end_point(a);
	double radius = game->beatmap.difficulty.circle_radius;
	double interval = 15;
	double center_distance = cabs(b->p - a_end);
	double edge_distance = center_distance - 2 * radius;
	if (edge_distance < interval)
		return;
	int steps = edge_distance / interval;
	assert (steps >= 1);
	interval = edge_distance / steps; /* recalibrate */
	oshu_vector direction = (b->p - a_end) / center_distance;
	oshu_point start = a_end + direction * radius;
	oshu_vector step = direction * interval;
	for (int i = 0; i < steps; ++i)
		oshu_draw_texture(&game->display, &game->osu.connector, start + (i + .5) * step);
}

/**
 * Draw all the visible nodes from the beatmap, according to the current
 * position in the song.
 */
int osu_draw(struct oshu_game *game)
{
	osu_view(game);
	struct oshu_hit *cursor = oshu_look_hit_up(game, game->beatmap.difficulty.approach_time);
	struct oshu_hit *next = NULL;
	double now = game->clock.now;
	for (struct oshu_hit *hit = cursor; hit; hit = hit->previous) {
		if (!(hit->type & (OSHU_CIRCLE_HIT | OSHU_SLIDER_HIT)))
			continue;
		if (oshu_hit_end_time(hit) < now - game->beatmap.difficulty.approach_time)
			break;
		if (next && next->combo == hit->combo)
			connect_hits(game, hit, next);
		draw_hit(game, hit);
		next = hit;
	}
	oshu_reset_view(&game->display);
	return 0;
}

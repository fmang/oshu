/**
 * \file osu/draw.c
 * \ingroup osu
 *
 * \brief
 * Drawing routines specific to the Osu game mode.
 */

#include "beatmap/beatmap.h"
#include "game/game.h"
#include "game/screen.h"
#include "graphics/display.h"
#include "graphics/draw.h"

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

static void draw_cursor(struct oshu_game *game)
{
	int fireflies = sizeof(game->osu.mouse_history) / sizeof(*game->osu.mouse_history);
	game->osu.mouse_offset = (game->osu.mouse_offset + 1) % fireflies;
	game->osu.mouse_history[game->osu.mouse_offset] = oshu_get_mouse(&game->display);

	for (int i = 1; i <= fireflies; ++i) {
		int offset = (game->osu.mouse_offset + i) % fireflies;
		double ratio = (double) (i + 1) / (fireflies + 1);
		SDL_SetTextureAlphaMod(game->osu.cursor.texture, ratio * 255);
		oshu_draw_scaled_texture(
			&game->display, &game->osu.cursor,
			game->osu.mouse_history[offset],
			ratio
		);
	}
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
 * Display the metadata on top of the gaming screen.
 *
 * Metadata are drawn in white text and a translucent black background for
 * readability.
 *
 * The display is shown when the game in paused, and in the 6 first seconds of
 * the game. It fades out from the 5th second to the 6th, where it becomes
 * completely invisible.
 *
 * Every 3.5 second, the display is switched between Unicode and ASCII, with a
 * 0.2-second fade transititon.
 */
static void draw_metadata(struct oshu_game *game)
{
	double ratio;
	if (game->screen == &oshu_pause_screen)
		ratio = 1.;
	else if (game->clock.system < 5.)
		ratio = 1.;
	else if (game->clock.system < 6.)
		ratio = 6. - game->clock.system;
	else
		return;

	SDL_Rect frame = {
		.x = 0,
		.y = 0,
		.w = creal(game->display.view.size),
		.h = cimag(game->osu.metadata.size),
	};
	SDL_SetRenderDrawColor(game->display.renderer, 0, 0, 0, 128 * ratio);
	SDL_SetRenderDrawBlendMode(game->display.renderer, SDL_BLENDMODE_BLEND);
	SDL_RenderFillRect(game->display.renderer, &frame);

	double phase = game->clock.system / 3.5;
	double progression = fabs(((phase - (int) phase) - 0.5) * 2.);
	int has_unicode = game->osu.metadata_unicode.texture != NULL;
	int unicode = has_unicode ? (int) phase % 2 == 0 : 0;
	double transition = 1.;
	if (progression > .9 && has_unicode)
		transition = 1. - (progression - .9) * 10.;

	struct oshu_texture *meta = unicode ? &game->osu.metadata_unicode : &game->osu.metadata;
	SDL_SetTextureAlphaMod(meta->texture, ratio * transition * 255);
	oshu_draw_texture(&game->display, meta, 0);

	SDL_SetTextureAlphaMod(game->osu.stars.texture, ratio * 255);
	oshu_draw_texture(&game->display, &game->osu.stars, creal(game->display.view.size));
}

/**
 * Draw at the bottom of the screen a progress bar showing how far in the song
 * we are.
 */
static void draw_progression(struct oshu_game *game)
{
	double progression = game->clock.now / game->audio.music.duration;
	if (progression < 0)
		progression = 0;
	else if (progression > 1)
		progression = 1;

	double height = 4;
	SDL_Rect bar = {
		.x = 0,
		.y = cimag(game->display.view.size) - height,
		.w = progression * creal(game->display.view.size),
		.h = height,
	};
	SDL_SetRenderDrawColor(game->display.renderer, 255, 255, 255, 48);
	SDL_SetRenderDrawBlendMode(game->display.renderer, SDL_BLENDMODE_BLEND);
	SDL_RenderFillRect(game->display.renderer, &bar);
}

/**
 * Draw all the visible nodes from the beatmap, according to the current
 * position in the song.
 */
int osu_draw(struct oshu_game *game)
{
	draw_progression(game);
	draw_metadata(game);

	oshu_fit_view(&game->display.view, 640 + 480 * I);
	oshu_resize_view(&game->display.view, 512 + 384 * I);
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
	draw_cursor(game);
	return 0;
}

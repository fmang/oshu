/**
 * \file game/osu/draw.c
 * \ingroup osu
 *
 * \brief
 * Drawing routines specific to the Osu game mode.
 */

#include "beatmap/beatmap.h"
#include "game/game.h"
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

static void draw_hit_circle(struct oshu_game *game, struct oshu_hit *hit)
{
	struct oshu_display *display = &game->display;
	if (hit->state == OSHU_INITIAL_HIT || hit->state == OSHU_SLIDING_HIT) {
		assert (hit->color != NULL);
		oshu_draw_texture(display, &game->osu.circles[hit->color->index], hit->p);
		draw_hint(game, hit);
	} else if (hit->state == OSHU_GOOD_HIT) {
		oshu_draw_texture(display, &game->osu.good_mark, oshu_end_point(hit));
	} else if (hit->state == OSHU_MISSED_HIT) {
		oshu_draw_texture(display, &game->osu.bad_mark, oshu_end_point(hit));
	} else if (hit->state == OSHU_SKIPPED_HIT) {
		oshu_draw_texture(display, &game->osu.skip_mark, oshu_end_point(hit));
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
	} else if (hit->state == OSHU_GOOD_HIT) {
		oshu_draw_texture(&game->display, &game->osu.good_mark, oshu_end_point(hit));
	} else if (hit->state == OSHU_MISSED_HIT) {
		oshu_draw_texture(&game->display, &game->osu.bad_mark, oshu_end_point(hit));
	} else if (hit->state == OSHU_SKIPPED_HIT) {
		oshu_draw_texture(display, &game->osu.skip_mark, oshu_end_point(hit));
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
	oshu_point mouse = oshu_get_mouse(&game->display);
	oshu_point prev = game->osu.previous_mouse;
	prev += (prev - mouse) * 1.5; /* increase the trail */
	int fireflies = 5;
	for (int i = 0; i < fireflies; ++i) {
		double ratio = (double) (fireflies - i) / fireflies;
		SDL_SetTextureAlphaMod(game->osu.cursor.texture, ratio * 255);
		oshu_draw_scaled_texture(
			&game->display, &game->osu.cursor,
			((fireflies - i) * mouse + i * prev) / fireflies,
			ratio
		);
	}
	game->osu.previous_mouse = mouse;
}

/**
 * Draw all the visible nodes from the beatmap, according to the current
 * position in the song.
 */
int osu_draw(struct oshu_game *game)
{
	struct oshu_hit *cursor = oshu_look_hit_up(game, game->beatmap.difficulty.approach_time);
	double now = game->clock.now;
	for (struct oshu_hit *hit = cursor; hit; hit = hit->previous) {
		if (!(hit->type & (OSHU_CIRCLE_HIT | OSHU_SLIDER_HIT)))
			continue;
		if (oshu_hit_end_time(hit) < now - game->beatmap.difficulty.approach_time)
			break;
		draw_hit(game, hit);
	}
	draw_cursor(game);
	return 0;
}

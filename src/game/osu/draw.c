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

static void draw_hint(struct oshu_game *game, struct oshu_hit *hit)
{
	double now = game->clock.now;
	if (hit->time > now && hit->state == OSHU_INITIAL_HIT) {
		SDL_SetRenderDrawColor(game->display.renderer, 255, 128, 64, 255);
		double ratio = (double) (hit->time - now) / game->beatmap.difficulty.approach_time;
		double radius = game->beatmap.difficulty.circle_radius;
		oshu_draw_circle(
			&game->display, hit->p,
			radius + ratio * game->beatmap.difficulty.approach_size
		);
	}
}

static void draw_hit_circle(struct oshu_game *game, struct oshu_hit *hit)
{
	struct oshu_display *display = &game->display;
	struct oshu_beatmap *beatmap = &game->beatmap;
	double radius = beatmap->difficulty.circle_radius;
	if (hit->state == OSHU_INITIAL_HIT || hit->state == OSHU_SLIDING_HIT) {
		oshu_draw_texture(display, &game->osu.circle_texture, hit->p);
		draw_hint(game, hit);
	} else if (hit->state == OSHU_GOOD_HIT) {
		oshu_point p = oshu_end_point(hit);
		SDL_SetRenderDrawColor(display->renderer, 64, 255, 64, 255);
		oshu_draw_circle(display, p, radius / 3);
	} else if (hit->state == OSHU_MISSED_HIT) {
		SDL_SetRenderDrawColor(display->renderer, 255, 64, 64, 255);
		oshu_point p = oshu_end_point(hit);
		oshu_vector d = radius / 3. + radius / 3. * I;
		oshu_draw_line(display, p - d, p + d);
		oshu_draw_line(display, p - d * I, p + d * I);
	} else if (hit->state == OSHU_SKIPPED_HIT) {
		SDL_SetRenderDrawColor(display->renderer, 64, 64, 255, 255);
		oshu_point p = oshu_end_point(hit);
		oshu_vector a = radius / 3.;
		oshu_vector b = a * cexp(2. * M_PI / 3. * I);
		oshu_vector c = b * cexp(2. * M_PI / 3. * I);
		oshu_draw_line(display, p + a, p + b);
		oshu_draw_line(display, p + b, p + c);
		oshu_draw_line(display, p + c, p + a);
	}
}

static void draw_slider(struct oshu_game *game, struct oshu_hit *hit)
{
	struct oshu_display *display = &game->display;
	struct oshu_beatmap *beatmap = &game->beatmap;
	double now = game->clock.now;
	double radius = beatmap->difficulty.circle_radius;
	if (hit->state == OSHU_INITIAL_HIT || hit->state == OSHU_SLIDING_HIT) {
		if (hit->texture)
			oshu_draw_texture(&game->display, hit->texture, hit->p);
		draw_hint(game, hit);
		/* ball */
		double t = (now - hit->time) / hit->slider.duration;
		if (hit->state == OSHU_SLIDING_HIT) {
			SDL_SetRenderDrawColor(display->renderer, 196, 64, 0, 255);
			oshu_point ball = oshu_path_at(&hit->slider.path, t < 0 ? 0 : t);
			oshu_draw_circle(display, ball, radius / 2);
			oshu_draw_circle(display, ball, beatmap->difficulty.slider_tolerance);
		}
		oshu_point end = oshu_path_at(&hit->slider.path, 1);
		int rounds_left = hit->slider.repeat - (t <= 0 ? 0 : (int) t);
	}
}

static void draw_hit(struct oshu_game *game, struct oshu_hit *hit)
{
	if (hit->type & OSHU_SLIDER_HIT)
		draw_slider(game, hit);
	else if (hit->type & OSHU_CIRCLE_HIT)
		draw_hit_circle(game, hit);
}

static void connect_hits(struct oshu_game *game, struct oshu_hit *prev, struct oshu_hit *next)
{
	if (prev->state != OSHU_INITIAL_HIT && prev->state != OSHU_SLIDING_HIT)
		return;
	if (next->state != OSHU_INITIAL_HIT && next->state != OSHU_SLIDING_HIT)
		return;
	SDL_SetRenderDrawColor(game->display.renderer, 0, 128, 196, 255);
	oshu_point end = oshu_end_point(prev);
	oshu_vector diff = next->p - end;
	oshu_vector d = oshu_normalize(diff);
	d *= game->beatmap.difficulty.circle_radius;
	oshu_draw_line(&game->display, end + d, next->p - d);
}

/**
 * Draw all the visible nodes from the beatmap, according to the current
 * position in the song.
 */
int osu_draw(struct oshu_game *game)
{
	struct oshu_hit *cursor = oshu_look_hit_up(game, game->beatmap.difficulty.approach_time);
	struct oshu_hit *prev = NULL;
	double now = game->clock.now;
	for (struct oshu_hit *hit = cursor; hit; hit = hit->previous) {
		if (!(hit->type & (OSHU_CIRCLE_HIT | OSHU_SLIDER_HIT)))
			continue;
		if (oshu_hit_end_time(hit) < now - game->beatmap.difficulty.approach_time)
			break;
		if (prev && !(prev->type & OSHU_NEW_HIT_COMBO))
			connect_hits(game, hit, prev);
		draw_hit(game, hit);
		prev = hit;
	}
	return 0;
}

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

static void draw_hit_circle(struct oshu_display *display, struct oshu_beatmap *beatmap, struct oshu_hit *hit, double now)
{
	double radius = beatmap->difficulty.circle_radius;
	if (hit->state == OSHU_INITIAL_HIT || hit->state == OSHU_SLIDING_HIT) {
		SDL_SetRenderDrawColor(display->renderer, 255, 255, 255, 255);
		double xradius = radius * .9;
		oshu_draw_circle(display, hit->p, radius);
		oshu_draw_circle(display, hit->p, xradius);
		oshu_draw_line(display, hit->p - xradius, hit->p + xradius);
		oshu_draw_line(display, hit->p - xradius * I, hit->p + xradius * I);
		if (hit->time > now) {
			/* hint circle */
			SDL_SetRenderDrawColor(display->renderer, 255, 128, 64, 255);
			double ratio = (double) (hit->time - now) / beatmap->difficulty.approach_time;
			oshu_draw_circle(display, hit->p, radius + ratio * beatmap->difficulty.approach_size);
		}
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

static void draw_slider(struct oshu_display *display, struct oshu_beatmap *beatmap, struct oshu_hit *hit, double now)
{
	double radius = beatmap->difficulty.circle_radius;
	draw_hit_circle(display, beatmap, hit, now);
	if (hit->state == OSHU_INITIAL_HIT || hit->state == OSHU_SLIDING_HIT) {
		double t = (now - hit->time) / hit->slider.duration;
		SDL_SetRenderDrawColor(display->renderer, 255, 255, 255, 255);
		oshu_draw_thick_path(display, &hit->slider.path, 2 * radius);
		if (hit->state == OSHU_SLIDING_HIT) {
			SDL_SetRenderDrawColor(display->renderer, 255, 255, 0, 255);
			oshu_point ball = oshu_path_at(&hit->slider.path, t < 0 ? 0 : t);
			oshu_draw_circle(display, ball, radius / 2);
			oshu_draw_circle(display, ball, beatmap->difficulty.slider_tolerance);
		}
		oshu_point end = oshu_path_at(&hit->slider.path, 1);
		int rounds_left = hit->slider.repeat - (t <= 0 ? 0 : (int) t);
		SDL_SetRenderDrawColor(display->renderer, 255, 255, 255, 255);
		for (int i = 1; i <= rounds_left; ++i)
			oshu_draw_circle(display, end, radius * ((double) i / rounds_left));
	}
}

static void draw_hit(struct oshu_display *display, struct oshu_beatmap *beatmap, struct oshu_hit *hit, double now)
{
	if (hit->type & OSHU_SLIDER_HIT)
		draw_slider(display, beatmap, hit, now);
	else if (hit->type & OSHU_CIRCLE_HIT)
		draw_hit_circle(display, beatmap, hit, now);
}

static void connect_hits(struct oshu_display *display, struct oshu_beatmap *beatmap, struct oshu_hit *prev, struct oshu_hit *next)
{
	if (prev->state != OSHU_INITIAL_HIT && prev->state != OSHU_SLIDING_HIT)
		return;
	if (next->state != OSHU_INITIAL_HIT && next->state != OSHU_SLIDING_HIT)
		return;
	SDL_SetRenderDrawColor(display->renderer, 0, 128, 196, 255);
	oshu_point end = oshu_end_point(prev);
	oshu_vector diff = next->p - end;
	oshu_vector d = oshu_normalize(diff);
	d *= beatmap->difficulty.circle_radius;
	oshu_draw_line(display, end + d, next->p - d);
}

/**
 * Draw all the visible nodes from the beatmap, according to the current
 * position in the song.
 *
 * `now` is the current position in the playing song, in seconds.
 */
static void draw_beatmap(struct oshu_display *display, struct oshu_beatmap *beatmap, struct oshu_hit *cursor, double now)
{
	struct oshu_hit *prev = NULL;
	for (struct oshu_hit *hit = cursor; hit; hit = hit->next) {
		if (!(hit->type & (OSHU_CIRCLE_HIT | OSHU_SLIDER_HIT)))
			continue;
		if (hit->time > now + beatmap->difficulty.approach_time)
			break;
		if (prev && !(hit->type & OSHU_NEW_HIT_COMBO))
			connect_hits(display, beatmap, prev, hit);
		draw_hit(display, beatmap, hit, now);
		prev = hit;
	}
}

int osu_draw(struct oshu_game *game)
{
	struct oshu_hit *start = oshu_look_hit_back(game, game->beatmap.difficulty.approach_time);
	draw_beatmap(&game->display, &game->beatmap, start, game->clock.now);
	/* XXX Toying around */
	oshu_draw_texture(&game->display, 0, &game->osu.circle_texture);
	SDL_RenderPresent(game->display.renderer);
	return 0;
}

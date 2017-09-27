/**
 * \file graphics/draw.c
 * \ingroup draw
 */

#include "graphics/draw.h"

/**
 * Handy alias to write point literals, like `(P) {x, y}`.
 *
 * We don't want that kind of shortcut to propagate outside of this module
 * though.
 */
typedef struct oshu_point P;

void oshu_draw_circle(struct oshu_display *display, struct oshu_point center, double radius)
{
	int resolution = 32;
	SDL_Point points[resolution];
	double step = 2 * M_PI / (resolution - 1);
	for (int i = 0; i < resolution; i++) {
		struct oshu_point p = oshu_project(display, (P) {
			.x = center.x + radius * cos(i * step),
			.y = center.y + radius * sin(i * step),
		});
		points[i] = (SDL_Point) {p.x, p.y};
	}
	SDL_RenderDrawLines(display->renderer, points, resolution);
}


void oshu_draw_line(struct oshu_display *display, struct oshu_point p1, struct oshu_point p2)
{
	p1 = oshu_project(display, p1);
	p2 = oshu_project(display, p2);
	SDL_RenderDrawLine(display->renderer, p1.x, p1.y, p2.x, p2.y);
}

static void draw_hit_circle(struct oshu_display *display, struct oshu_beatmap *beatmap, struct oshu_hit *hit, double now)
{
	double radius = beatmap->difficulty.circle_radius;
	if (hit->state == OSHU_HIT_INITIAL || hit->state == OSHU_HIT_SLIDING) {
		SDL_SetRenderDrawColor(display->renderer, 255, 255, 255, 255);
		double xradius = radius * .9;
		oshu_draw_circle(display, hit->p, radius);
		oshu_draw_circle(display, hit->p, xradius);
		oshu_draw_line(display, (P) {hit->p.x - xradius, hit->p.y}, (P) {hit->p.x + xradius, hit->p.y});
		oshu_draw_line(display, (P) {hit->p.x, hit->p.y - xradius}, (P) {hit->p.x, hit->p.y + xradius});
		if (hit->time > now) {
			/* hint circle */
			SDL_SetRenderDrawColor(display->renderer, 255, 128, 64, 255);
			double ratio = (double) (hit->time - now) / beatmap->difficulty.approach_time;
			oshu_draw_circle(display, hit->p, radius + ratio * beatmap->difficulty.approach_size);
		}
	} else if (hit->state == OSHU_HIT_GOOD) {
		struct oshu_point p = oshu_end_point(hit);
		SDL_SetRenderDrawColor(display->renderer, 64, 255, 64, 255);
		oshu_draw_circle(display, p, radius / 3);
	} else if (hit->state == OSHU_HIT_MISSED) {
		SDL_SetRenderDrawColor(display->renderer, 255, 64, 64, 255);
		struct oshu_point p = oshu_end_point(hit);
		int d = radius / 3;
		oshu_draw_line(display, (P) {p.x - d, p.y - d}, (P) {p.x + d, p.y + d});
		oshu_draw_line(display, (P) {p.x + d, p.y - d}, (P) {p.x - d, p.y + d});
	}
}

static void draw_slider(struct oshu_display *display, struct oshu_beatmap *beatmap, struct oshu_hit *hit, double now)
{
	double radius = beatmap->difficulty.circle_radius;
	draw_hit_circle(display, beatmap, hit, now);
	if (hit->state == OSHU_HIT_INITIAL || hit->state == OSHU_HIT_SLIDING) {
		double t = (now - hit->time) / hit->slider.duration;
		SDL_SetRenderDrawColor(display->renderer, 255, 255, 255, 255);
		oshu_draw_thick_path(display, &hit->slider.path, 2 * radius);
		if (hit->state == OSHU_HIT_SLIDING) {
			SDL_SetRenderDrawColor(display->renderer, 255, 255, 0, 255);
			struct oshu_point ball = oshu_path_at(&hit->slider.path, t < 0 ? 0 : t);
			oshu_draw_circle(display, ball, radius / 2);
			oshu_draw_circle(display, ball, beatmap->difficulty.slider_tolerance);
		}
		struct oshu_point end = oshu_path_at(&hit->slider.path, 1);
		int rounds_left = hit->slider.repeat - (t <= 0 ? 0 : (int) t);
		SDL_SetRenderDrawColor(display->renderer, 255, 255, 255, 255);
		for (int i = 1; i <= rounds_left; ++i)
			oshu_draw_circle(display, end, radius * ((double) i / rounds_left));
	}
}

void oshu_draw_hit(struct oshu_display *display, struct oshu_beatmap *beatmap, struct oshu_hit *hit, double now)
{
	if (hit->type & OSHU_HIT_SLIDER && hit->slider.path.type)
		draw_slider(display, beatmap, hit, now);
	else
		draw_hit_circle(display, beatmap, hit, now);
}

void oshu_draw_path(struct oshu_display *display, struct oshu_path *path)
{
	static int resolution = 30;
	SDL_Point points[resolution];
	double step = 1. / (resolution - 1);
	for (int i = 0; i < resolution; i++) {
		struct oshu_point p = oshu_project(display, oshu_path_at(path, i * step));
		points[i] = (SDL_Point) {p.x, p.y};
	}
	SDL_RenderDrawLines(display->renderer, points, resolution);
}

void oshu_draw_thick_path(struct oshu_display *display, struct oshu_path *path, double width)
{
	static int resolution = 100;
	SDL_Point left[resolution];
	SDL_Point right[resolution];
	double step = 1. / (resolution - 1);
	double radius = width / 2.;
	for (int i = 0; i < resolution; i++) {
		struct oshu_point p = oshu_path_at(path, i * step);
		struct oshu_vector d = oshu_path_derive(path, i * step);
		d = oshu_normalize(d);
		struct oshu_point l = oshu_project(display, (P) {
			.x = p.x - radius* d.y,
			.y = p.y + radius* d.x,
		});
		struct oshu_point r= oshu_project(display, (P) {
			.x = p.x + radius * d.y,
			.y = p.y - radius * d.x,
		});
		left[i] = (SDL_Point) {l.x, l.y};
		right[i] = (SDL_Point) {r.x, r.y};
	}
	SDL_RenderDrawLines(display->renderer, left, resolution);
	SDL_RenderDrawLines(display->renderer, right, resolution);
}

static void connect_hits(struct oshu_display *display, struct oshu_beatmap *beatmap, struct oshu_hit *prev, struct oshu_hit *next)
{
	if (prev->state != OSHU_HIT_INITIAL && prev->state != OSHU_HIT_SLIDING)
		return;
	if (next->state != OSHU_HIT_INITIAL && next->state != OSHU_HIT_SLIDING)
		return;
	SDL_SetRenderDrawColor(display->renderer, 0, 128, 196, 255);
	struct oshu_point end = oshu_end_point(prev);
	struct oshu_vector diff = { next->p.x - end.x, next->p.y - end.y };
	struct oshu_vector d = oshu_normalize(diff);
	d.x *= beatmap->difficulty.circle_radius;
	d.y *= beatmap->difficulty.circle_radius;
	oshu_draw_line(display, (P) {end.x + d.x, end.y + d.y}, (P) {next->p.x - d.x, next->p.y - d.y});
}

void oshu_draw_beatmap(struct oshu_display *display, struct oshu_beatmap *beatmap, struct oshu_hit *cursor, double now)
{
	struct oshu_hit *prev = NULL;
	for (struct oshu_hit *hit = cursor; hit; hit = hit->next) {
		if (hit->time > now + beatmap->difficulty.approach_time)
			break;
		if (prev && !(hit->type & OSHU_HIT_NEW_COMBO))
			connect_hits(display, beatmap, prev, hit);
		oshu_draw_hit(display, beatmap, hit, now);
		prev = hit;
	}
	SDL_RenderPresent(display->renderer);
}

void oshu_draw_background(struct oshu_display *display, SDL_Texture *pic)
{
	SDL_Rect dest;
	int ww, wh;
	int tw, th;
	SDL_GetWindowSize(display->window, &ww, &wh);
	SDL_QueryTexture(pic, NULL, NULL, &tw, &th);
	double window_ratio = (double) ww / wh;
	double pic_ratio = (double) tw / th;

	if (window_ratio > pic_ratio) {
		/* the window is too wide */
		dest.w = ww;
		dest.h = dest.w / pic_ratio;
		dest.x = 0;
		dest.y = (wh - dest.h) / 2;
	} else {
		/* the window is too high */
		dest.h = wh;
		dest.w = dest.h * pic_ratio;
		dest.y = 0;
		dest.x = (ww - dest.w) / 2;
	}
	SDL_RenderCopy(display->renderer, pic, NULL, &dest);
}

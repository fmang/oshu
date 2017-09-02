/**
 * \file graphics/draw.c
 * \ingroup draw
 */

#include "graphics/draw.h"

static void view_xy(struct oshu_display *display, int *x, int *y)
{
	*x = (64 + *x) * display->zoom + display->horizontal_margin;
	*y = (48 + *y) * display->zoom + display->vertical_margin;
}

/**
 * Translate a point from game zone coordinates to window coordinates.
 */
static void view_point(struct oshu_display *display, SDL_Point *p)
{
	view_xy(display, &p->x, &p->y);
}

struct oshu_point oshu_get_mouse(struct oshu_display *display)
{
	struct oshu_point p;
	int x, y;
	SDL_GetMouseState(&x, &y);
	p.x = (x - display->horizontal_margin) / display->zoom - 64.;
	p.y = (y - display->vertical_margin) / display->zoom - 48.;
	return p;
}

void oshu_draw_circle(struct oshu_display *display, double x, double y, double radius)
{
	int resolution = (int) radius;
	SDL_Point points[resolution];
	double step = 2 * M_PI / (resolution - 1);
	for (int i = 0; i < resolution; i++) {
		points[i].x = x + radius * cos(i * step);
		points[i].y = y + radius * sin(i * step);
		view_point(display, &points[i]);
	}
	SDL_RenderDrawLines(display->renderer, points, resolution);
}


void oshu_draw_line(struct oshu_display *display, int x1, int y1, int x2, int y2)
{
	view_xy(display, &x1, &y1);
	view_xy(display, &x2, &y2);
	SDL_RenderDrawLine(display->renderer, x1, y1, x2, y2);
}

static void draw_hit_circle(struct oshu_display *display, struct oshu_beatmap *beatmap, struct oshu_hit *hit, double now)
{
	double radius = beatmap->difficulty.circle_radius;
	if (hit->state == OSHU_HIT_INITIAL || hit->state == OSHU_HIT_SLIDING) {
		SDL_SetRenderDrawColor(display->renderer, 255, 255, 255, 255);
		oshu_draw_circle(display, hit->p.x, hit->p.y, radius);
		oshu_draw_circle(display, hit->p.x, hit->p.y, radius - 2);
		oshu_draw_line(display, hit->p.x - radius, hit->p.y, hit->p.x + radius, hit->p.y);
		oshu_draw_line(display, hit->p.x, hit->p.y - radius, hit->p.x, hit->p.y + radius);
		if (hit->time > now) {
			/* hint circle */
			SDL_SetRenderDrawColor(display->renderer, 255, 128, 64, 255);
			double ratio = (double) (hit->time - now) / beatmap->difficulty.approach_time;
			oshu_draw_circle(display, hit->p.x, hit->p.y, radius + ratio * beatmap->difficulty.approach_size);
		}
	} else if (hit->state == OSHU_HIT_GOOD) {
		struct oshu_point p = oshu_end_point(hit);
		SDL_SetRenderDrawColor(display->renderer, 64, 255, 64, 255);
		oshu_draw_circle(display, p.x, p.y, radius / 3);
	} else if (hit->state == OSHU_HIT_MISSED) {
		SDL_SetRenderDrawColor(display->renderer, 255, 64, 64, 255);
		struct oshu_point p = oshu_end_point(hit);
		int d = radius / 3;
		oshu_draw_line(display, p.x - d, p.y - d, p.x + d, p.y + d);
		oshu_draw_line(display, p.x + d, p.y - d, p.x - d, p.y + d);
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
			oshu_draw_circle(display, ball.x, ball.y, radius / 2);
			oshu_draw_circle(display, ball.x, ball.y, beatmap->difficulty.slider_tolerance);
		}
		struct oshu_point end = oshu_path_at(&hit->slider.path, 1);
		int rounds_left = hit->slider.repeat - (t <= 0 ? 0 : (int) t);
		SDL_SetRenderDrawColor(display->renderer, 255, 255, 255, 255);
		for (int i = 1; i <= rounds_left; ++i)
			oshu_draw_circle(display, end.x, end.y, radius * ((double) i / rounds_left));
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
		struct oshu_point p = oshu_path_at(path, i * step);
		points[i].x = p.x;
		points[i].y = p.y;
		view_point(display, &points[i]);
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
		left[i].x = p.x - radius* d.y;
		left[i].y = p.y + radius* d.x;
		right[i].x = p.x + radius * d.y;
		right[i].y = p.y - radius * d.x;
		view_point(display, &left[i]);
		view_point(display, &right[i]);
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
	oshu_draw_line(display, end.x + d.x, end.y + d.y, next->p.x - d.x, next->p.y - d.y);
}

void oshu_draw_beatmap(struct oshu_display *display, struct oshu_beatmap *beatmap, double now)
{
	SDL_SetRenderDrawColor(display->renderer, 0, 0, 0, 255);
	SDL_RenderClear(display->renderer);
	struct oshu_hit *prev = NULL;
	for (struct oshu_hit *hit = beatmap->hit_cursor; hit; hit = hit->next) {
		if (hit->time > now + beatmap->difficulty.approach_time)
			break;
		if (prev && !(hit->type & OSHU_HIT_NEW_COMBO))
			connect_hits(display, beatmap, prev, hit);
		oshu_draw_hit(display, beatmap, hit, now);
		prev = hit;
	}
	SDL_RenderPresent(display->renderer);
}

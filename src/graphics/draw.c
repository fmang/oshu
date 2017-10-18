/**
 * \file graphics/draw.c
 * \ingroup draw
 */

#include "graphics/draw.h"

#include <assert.h>

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
		if (d.x == 0 && d.y == 0) {
			/* degenerate case */
			assert (i != 0);
			left[i] = left[i-1];
			right[i] = right[i-1];
			continue;
		}
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

/**
 * \file graphics/draw.c
 * \ingroup draw
 */

#include "graphics/draw.h"

#include "graphics/display.h"

#include <assert.h>
#include <SDL2/SDL.h>

void oshu_draw_circle(struct oshu_display *display, oshu_point center, double radius)
{
	int resolution = 32;
	SDL_Point points[resolution];
	double step = 2 * M_PI / (resolution - 1);
	for (int i = 0; i < resolution; i++) {
		oshu_point p = oshu_project(display, center + radius * cexp(i * step * I));
		points[i] = (SDL_Point) {creal(p), cimag(p)};
	}
	SDL_RenderDrawLines(display->renderer, points, resolution);
}


void oshu_draw_line(struct oshu_display *display, oshu_point p1, oshu_point p2)
{
	p1 = oshu_project(display, p1);
	p2 = oshu_project(display, p2);
	SDL_RenderDrawLine(display->renderer, creal(p1), cimag(p1), creal(p2), cimag(p2));
}

void oshu_draw_path(struct oshu_display *display, struct oshu_path *path)
{
	static int resolution = 30;
	SDL_Point points[resolution];
	double step = 1. / (resolution - 1);
	for (int i = 0; i < resolution; i++) {
		oshu_point p = oshu_project(display, oshu_path_at(path, i * step));
		points[i] = (SDL_Point) {creal(p), cimag(p)};
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
		oshu_point p = oshu_path_at(path, i * step);
		oshu_vector d = oshu_path_derive(path, i * step);
		d = oshu_normalize(d);
		if (d == 0) {
			/* degenerate case */
			assert (i != 0);
			left[i] = left[i-1];
			right[i] = right[i-1];
			continue;
		}
		oshu_point l = oshu_project(display, p + radius * d * I);
		oshu_point r = oshu_project(display, p - radius * d * I);
		left[i] = (SDL_Point) {creal(l), cimag(l)};
		right[i] = (SDL_Point) {creal(r), cimag(r)};
	}
	SDL_RenderDrawLines(display->renderer, left, resolution);
	SDL_RenderDrawLines(display->renderer, right, resolution);
}

void oshu_draw_background(struct oshu_display *display, struct oshu_texture *pic)
{
	SDL_Rect dest;
	int ww, wh;
	SDL_GetWindowSize(display->window, &ww, &wh);
	double window_ratio = (double) ww / wh;
	double pic_ratio = oshu_ratio(pic->size);

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
	SDL_RenderCopy(display->renderer, pic->texture, NULL, &dest);
}

void oshu_draw_texture(struct oshu_display *display, struct oshu_texture *texture, oshu_point p)
{
	oshu_point top_left = oshu_project(display, p - texture->origin);
	oshu_size size = texture->size * display->view.zoom;
	SDL_Rect dest = {
		.x = creal(top_left), .y = cimag(top_left),
		.w = creal(size), .h = cimag(size),
	};
	SDL_RenderCopy(display->renderer, texture->texture, NULL, &dest);
}

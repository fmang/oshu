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

void oshu_draw_scaled_texture(struct oshu_display *display, struct oshu_texture *texture, oshu_point p, double ratio)
{
	oshu_point top_left = oshu_project(display, p - texture->origin * ratio);
	oshu_size size = texture->size * ratio * display->view.zoom;
	SDL_Rect dest = {
		.x = creal(top_left), .y = cimag(top_left),
		.w = creal(size), .h = cimag(size),
	};
	SDL_RenderCopy(display->renderer, texture->texture, NULL, &dest);
}

void oshu_draw_texture(struct oshu_display *display, struct oshu_texture *texture, oshu_point p)
{
	oshu_draw_scaled_texture(display, texture, p, 1.);
}

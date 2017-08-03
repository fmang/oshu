#include "graphics.h"
#include "log.h"

#include <SDL2/SDL_image.h>

static const int window_width = 640; /* px */
static const int window_height = 480; /* px */

static const int game_width = 512;
static const int game_height = 384;

static const int hit_radius = 32; /* px */

int create_window(struct oshu_display *display)
{
	display->window = SDL_CreateWindow(
		"Oshu!",
		SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
		window_width, window_height,
		0
	);
	if (display->window == NULL)
		goto fail;
	display->renderer = SDL_CreateRenderer(display->window, -1, 0);
	if (display->renderer == NULL)
		goto fail;
	return 0;
fail:
	oshu_log_error("error creating the display: %s", SDL_GetError());
	return -1;
}

int load_textures(struct oshu_display *display)
{
	display->hit_mark = IMG_LoadTexture(display->renderer, "hit.png");
	if (display->hit_mark == NULL) {
		oshu_log_error("error load the textures: %s", SDL_GetError());
		return -1;
	}
	return 0;
}

int oshu_display_init(struct oshu_display **display)
{
	*display = calloc(1, sizeof(**display));
	if (create_window(*display) < 0)
		goto fail;
	if (load_textures(*display) < 0)
		goto fail;
	return 0;
fail:
	oshu_display_destroy(display);
	return -1;
}

void oshu_display_destroy(struct oshu_display **display)
{
	if ((*display)->renderer)
		SDL_DestroyRenderer((*display)->renderer);
	if ((*display)->window)
		SDL_DestroyWindow((*display)->window);
	free(*display);
	*display = NULL;
}

/**
 * Translate a point from game zone coordinates to window coordinates.
 */
static void view_point(struct oshu_display *display, SDL_Point *p)
{
	p->x += (window_width - game_width) / 2;
	p->y += (window_height - game_height) / 2;
}

/**
 * Translate a rectangle from game zone coordinates to window coordinates.
 */
static void view_rect(struct oshu_display *display, SDL_Rect *r)
{
	r->x += (window_width - game_width) / 2;
	r->y += (window_height - game_height) / 2;
}

void oshu_draw_circle(struct oshu_display *display, double x, double y, double radius)
{
	static int resolution = 30;
	SDL_Point points[resolution];
	double step = 2 * M_PI / (resolution - 1);
	for (int i = 0; i < resolution; i++) {
		points[i].x = x + radius * cos(i * step);
		points[i].y = y + radius * sin(i * step);
		view_point(display, &points[i]);
	}
	SDL_RenderDrawLines(display->renderer, points, resolution);
}

void oshu_draw_hit(struct oshu_display *display, struct oshu_hit *hit)
{
	SDL_Rect where;
	where.x = hit->x - hit_radius;
	where.y = hit->y - hit_radius;
	where.w = hit_radius * 2;
	where.h = hit_radius * 2;
	view_rect(display, &where);
	SDL_RenderCopy(display->renderer, display->hit_mark, NULL, &where);
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
		struct oshu_point d = oshu_path_derive(path, i * step);
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

void oshu_draw_beatmap(struct oshu_display *display, struct oshu_beatmap *beatmap, int msecs)
{
	SDL_SetRenderDrawColor(display->renderer, 0, 0, 0, 255);
	SDL_RenderClear(display->renderer);
	SDL_SetRenderDrawColor(display->renderer, 255, 255, 255, 255);
	if (beatmap->hit_cursor)
		oshu_draw_hit(display, beatmap->hit_cursor);
	SDL_RenderPresent(display->renderer);
}

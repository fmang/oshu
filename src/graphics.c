/**
 * \file graphics.c
 * \ingroup graphics
 */

#include "graphics.h"
#include "log.h"

static const int game_width = 640; /* px */
static const int game_height = 480; /* px */

int create_window(struct oshu_display *display)
{
	display->window = SDL_CreateWindow(
		"oshu!",
		SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
		game_width, game_height,
		SDL_WINDOW_RESIZABLE
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
	return 0;
}

int oshu_display_init(struct oshu_display **display)
{
	*display = calloc(1, sizeof(**display));
	(*display)->zoom = 1;
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

void oshu_display_resize(struct oshu_display *display, int w, int h)
{
	double original_ratio = (double) game_width / game_height;
	double actual_ratio = (double) w / h;
	if (actual_ratio > original_ratio) {
		/* the window is too wide */
		display->zoom = (double) h / game_height;
		display->horizontal_margin = (w - game_width * display->zoom) / 2;
		display->vertical_margin = 0;
	} else {
		/* the window is too high */
		display->zoom = (double) w / game_width;
		display->horizontal_margin = 0;
		display->vertical_margin = (h - game_height * display->zoom) / 2;
	}
}

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

void oshu_get_mouse(struct oshu_display *display, int *x, int *y)
{
	SDL_GetMouseState(x, y);
	*x = (*x - display->horizontal_margin) / display->zoom - 64;
	*y = (*y - display->vertical_margin) / display->zoom - 48;
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


void oshu_draw_line(struct oshu_display *display, int x1, int y1, int x2, int y2)
{
	view_xy(display, &x1, &y1);
	view_xy(display, &x2, &y2);
	SDL_RenderDrawLine(display->renderer, x1, y1, x2, y2);
}

void oshu_draw_hit(struct oshu_display *display, struct oshu_beatmap *beatmap, struct oshu_hit *hit, double now)
{
	double radius = beatmap->difficulty.circle_radius;
	if (hit->state == OSHU_HIT_INITIAL) {
		SDL_SetRenderDrawColor(display->renderer, 255, 255, 255, 255);
		oshu_draw_circle(display, hit->x, hit->y, radius);
		oshu_draw_circle(display, hit->x, hit->y, radius - 2);
		oshu_draw_line(display, hit->x - radius, hit->y, hit->x + radius, hit->y);
		oshu_draw_line(display, hit->x, hit->y - radius, hit->x, hit->y + radius);
		if (hit->time > now) {
			/* hint circle */
			double ratio = (double) (hit->time - now) / beatmap->difficulty.approach_rate;
			oshu_draw_circle(display, hit->x, hit->y, radius + ratio * (radius * 2));
		}
	} else if (hit->state == OSHU_HIT_GOOD) {
		SDL_SetRenderDrawColor(display->renderer, 64, 255, 64, 255);
		oshu_draw_circle(display, hit->x, hit->y, radius / 3);
	} else if (hit->state == OSHU_HIT_MISSED) {
		SDL_SetRenderDrawColor(display->renderer, 255, 64, 64, 255);
		int d = radius / 3;
		oshu_draw_line(display, hit->x - d, hit->y - d, hit->x + d, hit->y + d);
		oshu_draw_line(display, hit->x + d, hit->y - d, hit->x - d, hit->y + d);
	}
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

void oshu_draw_beatmap(struct oshu_display *display, struct oshu_beatmap *beatmap, double now)
{
	SDL_SetRenderDrawColor(display->renderer, 0, 0, 0, 255);
	SDL_RenderClear(display->renderer);
	struct oshu_hit *prev = NULL;
	for (struct oshu_hit *hit = beatmap->hit_cursor; hit; hit = hit->next) {
		if (hit->time > now + beatmap->difficulty.approach_rate)
			break;
		if (prev && !(hit->type & OSHU_HIT_NEW_COMBO)) {
			SDL_SetRenderDrawColor(display->renderer, 128, 128, 128, 255);
			oshu_draw_line(display, prev->x, prev->y, hit->x, hit->y);
		}
		oshu_draw_hit(display, beatmap, hit, now);
		prev = hit;
	}
	SDL_RenderPresent(display->renderer);
}

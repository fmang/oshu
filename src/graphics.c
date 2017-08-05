#include "graphics.h"
#include "log.h"

static const int window_width = 640; /* px */
static const int window_height = 480; /* px */

static const int game_width = 512;
static const int game_height = 384;

static const int hit_radius = 24; /* px */
static const int hit_hint = 64; /* px */

static const int hit_time_in = 1000; /* ms */
static const int hit_time_out = 0; /* ms */

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

static void view_xy(struct oshu_display *display, int *x, int *y)
{
	*x += (window_width - game_width) / 2;
	*y += (window_height - game_height) / 2;
}

/**
 * Translate a point from game zone coordinates to window coordinates.
 */
static void view_point(struct oshu_display *display, SDL_Point *p)
{
	view_xy(display, &p->x, &p->y);
}

/**
 * Translate a rectangle from game zone coordinates to window coordinates.
 */
static void view_rect(struct oshu_display *display, SDL_Rect *r)
{
	view_xy(display, &r->x, &r->y);
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

void oshu_draw_hit(struct oshu_display *display, struct oshu_hit *hit, int msecs)
{
	SDL_SetRenderDrawColor(display->renderer, 255, 255, 255, 255);
	oshu_draw_circle(display, hit->x, hit->y, hit_radius);
	oshu_draw_circle(display, hit->x, hit->y, hit_radius - 2);
	oshu_draw_line(display, hit->x - hit_radius, hit->y, hit->x + hit_radius, hit->y);
	oshu_draw_line(display, hit->x, hit->y - hit_radius, hit->x, hit->y + hit_radius);
	if (hit->time > msecs) {
		double ratio = (double) (hit->time - msecs) / hit_time_in;
		oshu_draw_circle(display, hit->x, hit->y, hit_radius + ratio * hit_hint);
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

void oshu_draw_beatmap(struct oshu_display *display, struct oshu_beatmap *beatmap, int msecs)
{
	SDL_SetRenderDrawColor(display->renderer, 0, 0, 0, 255);
	SDL_RenderClear(display->renderer);
	struct oshu_hit *prev = NULL;
	for (struct oshu_hit *hit = beatmap->hit_cursor; hit; hit = hit->next) {
		if (hit->time > msecs + hit_time_in)
			break;
		if (prev && !(hit->type & OSHU_HIT_NEW_COMBO)) {
			SDL_SetRenderDrawColor(display->renderer, 128, 128, 128, 255);
			oshu_draw_line(display, prev->x, prev->y, hit->x, hit->y);
		}
		if (hit->time > msecs - hit_time_out)
			oshu_draw_hit(display, hit, msecs);
		prev = hit;
	}
	SDL_RenderPresent(display->renderer);
}

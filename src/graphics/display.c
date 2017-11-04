/**
 * \file graphics/display.c
 * \ingroup display
 */

#include "graphics/display.h"
#include "log.h"

#include <assert.h>

void oshu_scale_view(struct oshu_view *view, double factor)
{
	view->zoom *= factor;
	view->width /= factor;
	view->height /= factor;
}

void oshu_resize_view(struct oshu_view *view, double w, double h)
{
	view->origin += view->zoom * (view->width - w) / 2.;
	view->origin += view->zoom * (view->height - h) / 2. * I;
	view->width = w;
	view->height = h;
}

void oshu_fit_view(struct oshu_view *view, double w, double h)
{
	double wanted_ratio = w / h;
	double current_ratio = view->width / view->height;
	if (current_ratio > wanted_ratio) {
		/* the window is too wide */
		oshu_scale_view(view, view->height / h);
	} else {
		/* the window is too high */
		oshu_scale_view(view, view->width / w);
	}
	oshu_resize_view(view, w, h);
}

oshu_point oshu_project(struct oshu_display *display, oshu_point p)
{
	return p * display->view.zoom + display->view.origin;
}

oshu_point oshu_unproject(struct oshu_display *display, oshu_point p)
{
	return (p - display->view.origin) / display->view.zoom;
}

/**
 * Open the window and create the rendered.
 *
 * The default window size, 960x720 is arbitrary but proportional the the game
 * area. It's just a saner default for most screens.
 */
int create_window(struct oshu_display *display)
{
	display->window = SDL_CreateWindow(
		"oshu!",
		SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
		960, 720,
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

int oshu_open_display(struct oshu_display *display)
{
	if (create_window(display) < 0)
		goto fail;
	oshu_reset_view(display);
	return 0;
fail:
	oshu_close_display(display);
	return -1;
}

void oshu_close_display(struct oshu_display *display)
{
	if (display->renderer) {
		SDL_DestroyRenderer(display->renderer);
		display->renderer = NULL;
	}
	if (display->window) {
		SDL_DestroyWindow(display->window);
		display->window = NULL;
	}
}

void oshu_reset_view(struct oshu_display *display)
{
	int w, h;
	SDL_GetWindowSize(display->window, &w, &h);
	display->view.zoom = 1.;
	display->view.origin = 0;
	display->view.width = w;
	display->view.height = h;
}

oshu_point oshu_get_mouse(struct oshu_display *display)
{
	int x, y;
	SDL_GetMouseState(&x, &y);
	return oshu_unproject(display, x + y * I);
}

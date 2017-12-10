/**
 * \file graphics/view.c
 * \ingroup graphics_view
 */

#include "graphics/view.h"

#include "graphics/display.h"

#include <SDL2/SDL.h>

void oshu_scale_view(struct oshu_view *view, double factor)
{
	view->zoom *= factor;
	view->size /= factor;
}

void oshu_resize_view(struct oshu_view *view, oshu_size size)
{
	view->origin += view->zoom * (view->size - size) / 2.;
	view->size = size;
}

void oshu_fit_view(struct oshu_view *view, oshu_size size)
{
	double wanted_ratio = oshu_ratio(size);
	double current_ratio = oshu_ratio(view->size);
	if (current_ratio > wanted_ratio) {
		/* the window is too wide */
		oshu_scale_view(view, cimag(view->size) / cimag(size));
	} else {
		/* the window is too high */
		oshu_scale_view(view, creal(view->size) / creal(size));
	}
	oshu_resize_view(view, size);
}

oshu_point oshu_project(struct oshu_display *display, oshu_point p)
{
	return p * display->view.zoom + display->view.origin;
}

oshu_point oshu_unproject(struct oshu_display *display, oshu_point p)
{
	return (p - display->view.origin) / display->view.zoom;
}

void oshu_reset_view(struct oshu_display *display)
{
	int w, h;
	SDL_GetWindowSize(display->window, &w, &h);
	display->view.zoom = 1.;
	display->view.origin = 0;
	display->view.size = w + h * I;
}

/**
 * \file video/view.cc
 * \ingroup video_view
 */

#include "video/view.h"

#include "video/display.h"

#include <SDL2/SDL.h>

void oshu::scale_view(oshu::view *view, double factor)
{
	view->zoom *= factor;
	view->size /= factor;
}

void oshu::resize_view(oshu::view *view, oshu::size size)
{
	view->origin += view->zoom * (view->size - size) / 2.;
	view->size = size;
}

void oshu::fit_view(oshu::view *view, oshu::size size)
{
	double wanted_ratio = oshu::ratio(size);
	double current_ratio = oshu::ratio(view->size);
	if (current_ratio > wanted_ratio) {
		/* the window is too wide */
		oshu::scale_view(view, std::imag(view->size) / std::imag(size));
	} else {
		/* the window is too high */
		oshu::scale_view(view, std::real(view->size) / std::real(size));
	}
	oshu::resize_view(view, size);
}

oshu::point oshu::project(oshu::view *view, oshu::point p)
{
	return p * view->zoom + view->origin;
}

oshu::point oshu::unproject(oshu::view *view, oshu::point p)
{
	return (p - view->origin) / view->zoom;
}

void oshu::reset_view(oshu::display *display)
{
	int w, h;
	SDL_GetWindowSize(display->window, &w, &h);
	display->view.zoom = 1.;
	display->view.origin = 0;
	display->view.size = oshu::size(w, h);
}

/**
 * \file ui/cursor.cc
 * \ingroup ui_cursor
 */

#include "ui/cursor.h"

#include "video/display.h"
#include "video/paint.h"

#include <math.h>
#include <SDL2/SDL.h>

static int paint_cursor(oshu::cursor_widget *cursor)
{
	double radius = 14;
	oshu::size size = oshu::size{1, 1} * radius * 2.;

	oshu::painter p;
	oshu::start_painting(cursor->display, size, &p);
	cairo_translate(p.cr, radius, radius);

	cairo_pattern_t *pattern = cairo_pattern_create_radial(
		0, 0, 0,
		0, 0, radius - 1
	);
	cairo_pattern_add_color_stop_rgba(pattern, 0.0, 1, 1, 1, .8);
	cairo_pattern_add_color_stop_rgba(pattern, 0.6, 1, 1, 1, .8);
	cairo_pattern_add_color_stop_rgba(pattern, 0.7, 1, 1, 1, .3);
	cairo_pattern_add_color_stop_rgba(pattern, 1.0, 1, 1, 1, 0);
	cairo_arc(p.cr, 0, 0, radius - 1, 0, 2. * M_PI);
	cairo_set_source(p.cr, pattern);
	cairo_fill(p.cr);
	cairo_pattern_destroy(pattern);

	int rc = oshu::finish_painting(&p, &cursor->mouse);
	cursor->mouse.origin = size / 2.;
	return rc;
}

int oshu::create_cursor(oshu::display *display, oshu::cursor_widget *cursor)
{
	memset(cursor, 0, sizeof(*cursor));
	cursor->display = display;

	if (!(display->features & oshu::FANCY_CURSOR))
		return 0;

	int fireflies = sizeof(cursor->history) / sizeof(*cursor->history);
	oshu::point mouse = oshu::get_mouse(display);
	for (int i = 0; i < fireflies; ++i)
		cursor->history[i] = mouse;

	return paint_cursor(cursor);
}

void oshu::show_cursor(oshu::cursor_widget *cursor)
{
	if (!(cursor->display->features & oshu::FANCY_CURSOR))
		return;

	int fireflies = sizeof(cursor->history) / sizeof(*cursor->history);
	cursor->offset = (cursor->offset + 1) % fireflies;
	cursor->history[cursor->offset] = oshu::get_mouse(cursor->display);

	for (int i = 1; i <= fireflies; ++i) {
		int offset = (cursor->offset + i) % fireflies;
		double ratio = (double) (i + 1) / (fireflies + 1);
		SDL_SetTextureAlphaMod(cursor->mouse.texture, ratio * 255);
		oshu::draw_scaled_texture(
			cursor->display, &cursor->mouse,
			cursor->history[offset],
			ratio
		);
	}
}

void oshu::destroy_cursor(oshu::cursor_widget *cursor)
{
	if (!cursor->display)
		return;
	if (!(cursor->display->features & oshu::FANCY_CURSOR))
		return;

	oshu::destroy_texture(&cursor->mouse);
}

/**
 * \file lib/ui/osu.cc
 * \ingroup ui
 *
 * \brief
 * Drawing routines specific to the osu!standard game mode.
 */

#include "ui/osu.h"

#include "game/osu.h"
#include "video/display.h"
#include "video/texture.h"

#include <assert.h>

static void draw_hint(oshu::osu_ui &view, oshu::hit *hit)
{
	oshu::game_base *game = &view.game;
	double now = game->clock.now;
	if (hit->time > now && hit->state == oshu::INITIAL_HIT) {
		SDL_SetRenderDrawColor(view.display->renderer, 255, 128, 64, 255);
		double ratio = (double) (hit->time - now) / game->beatmap.difficulty.approach_time;
		double base_radius = game->beatmap.difficulty.circle_radius;
		double radius = base_radius + ratio * game->beatmap.difficulty.approach_size;
		oshu::draw_scaled_texture(
			view.display, &view.approach_circle, hit->p,
			2. * radius / std::real(view.approach_circle.size)
		);
	}
}

static void draw_hit_mark(oshu::osu_ui &view, oshu::hit *hit)
{
	oshu::game_base *game = &view.game;
	if (hit->state == oshu::GOOD_HIT) {
		double leniency = game->beatmap.difficulty.leniency;
		oshu::texture *mark = &view.good_mark;
		if (hit->offset < - leniency / 2)
			mark = &view.early_mark;
		else if (hit->offset > leniency / 2)
			mark = &view.late_mark;
		oshu::draw_texture(view.display, mark, oshu::end_point(hit));
	} else if (hit->state == oshu::MISSED_HIT) {
		oshu::draw_texture(view.display, &view.bad_mark, oshu::end_point(hit));
	} else if (hit->state == oshu::SKIPPED_HIT) {
		oshu::draw_texture(view.display, &view.skip_mark, oshu::end_point(hit));
	}
}

static void draw_hit_circle(oshu::osu_ui &view, oshu::hit *hit)
{
	oshu::display *display = view.display;
	if (hit->state == oshu::INITIAL_HIT) {
		assert (hit->color != NULL);
		oshu::draw_texture(display, &view.circles[hit->color->index], hit->p);
		draw_hint(view, hit);
	} else {
		draw_hit_mark(view, hit);
	}
}

static void draw_slider(oshu::osu_ui &view, oshu::hit *hit)
{
	oshu::game_base *game = &view.game;
	oshu::display *display = view.display;
	double now = game->clock.now;
	if (hit->state == oshu::INITIAL_HIT || hit->state == oshu::SLIDING_HIT) {
		if (!hit->texture) {
			oshu::osu_paint_slider(view, hit);
			assert (hit->texture != NULL);
		}
		oshu::draw_texture(view.display, hit->texture, hit->p);
		draw_hint(view, hit);
		/* ball */
		double t = (now - hit->time) / hit->slider.duration;
		if (hit->state == oshu::SLIDING_HIT) {
			oshu::point ball = oshu::path_at(&hit->slider.path, t < 0 ? 0 : t);
			oshu::draw_texture(display, &view.slider_ball, ball);
		}
	} else {
		draw_hit_mark(view, hit);
	}
}

static void draw_hit(oshu::osu_ui &view, oshu::hit *hit)
{
	if (hit->type & oshu::SLIDER_HIT)
		draw_slider(view, hit);
	else if (hit->type & oshu::CIRCLE_HIT)
		draw_hit_circle(view, hit);
}

/**
 * Connect two hits with a dotted line.
 *
 * ( ) · · · · ( )
 *
 * First, compute the visual distance between two hits, which is the distance
 * between the center, minus the two radii. Then split this interval in steps
 * of 15 pixels. Because we need an integral number of steps, floor it.
 *
 * The flooring would cause a extra padding after the last point, so we need to
 * recalibrate the interval by dividing the distance by the number of steps.
 *
 * Now we have our steps:
 *
 * ( )   |   |   |   |   ( )
 *
  However, for some reason, this yields an excessive visual margin before the
  first point and after the last point. To remedy this, the dots are put in the
  middle of the steps, instead of between.
 *
 * ( ) · | · | · | · | · ( )
 *
 * Voilà!
 *
 */
static void connect_hits(oshu::osu_ui &view, oshu::hit *a, oshu::hit *b)
{
	oshu::game_base *game = &view.game;
	if (a->state != oshu::INITIAL_HIT && a->state != oshu::SLIDING_HIT)
		return;
	oshu::point a_end = oshu::end_point(a);
	double radius = game->beatmap.difficulty.circle_radius;
	double interval = 15;
	double center_distance = std::abs(b->p - a_end);
	double edge_distance = center_distance - 2 * radius;
	if (edge_distance < interval)
		return;
	int steps = edge_distance / interval;
	assert (steps >= 1);
	interval = edge_distance / steps; /* recalibrate */
	oshu::vector direction = (b->p - a_end) / center_distance;
	oshu::point start = a_end + direction * radius;
	oshu::vector step = direction * interval;
	for (int i = 0; i < steps; ++i)
		oshu::draw_texture(view.display, &view.connector, start + (i + .5) * step);
}

namespace oshu {

/**
 * \todo
 * Handle errors.
 */
osu_ui::osu_ui(oshu::display *display, oshu::osu_game &game)
: display(display), game(game)
{
	assert (display != nullptr);
	oshu::osu_view(display);
	int rc = oshu::osu_paint_resources(*this);
	if (oshu::create_cursor(display, &cursor) < 0)
		rc = -1;
	oshu::reset_view(display);
	mouse = std::make_shared<osu_mouse>(display);
	game.mouse = mouse;
}

osu_ui::~osu_ui()
{
	SDL_ShowCursor(SDL_ENABLE);
	oshu::osu_free_resources(*this);
	oshu::destroy_cursor(&cursor);
}

/**
 * Draw all the visible nodes from the beatmap, according to the current
 * position in the song.
 */
void osu_ui::draw()
{
	oshu::osu_view(display);
	oshu::hit *cursor = oshu::look_hit_up(&game, game.beatmap.difficulty.approach_time);
	oshu::hit *next = NULL;
	double now = game.clock.now;
	for (oshu::hit *hit = cursor; hit; hit = hit->previous) {
		if (!(hit->type & (oshu::CIRCLE_HIT | oshu::SLIDER_HIT)))
			continue;
		if (oshu::hit_end_time(hit) < now - game.beatmap.difficulty.approach_time)
			break;
		if (next && next->combo == hit->combo)
			connect_hits(*this, hit, next);
		draw_hit(*this, hit);
		next = hit;
	}
	oshu::show_cursor(&this->cursor);
	oshu::reset_view(display);
}

osu_mouse::osu_mouse(oshu::display *display)
: display(display)
{
}

oshu::point osu_mouse::position()
{
	oshu::osu_view(display);
	oshu::point mouse = oshu::get_mouse(display);
	oshu::reset_view(display);
	return mouse;
}

}

void oshu::osu_view(oshu::display *display)
{
	oshu::fit_view(&display->view, oshu::size{640, 480});
	oshu::resize_view(&display->view, oshu::size{512, 384});
}

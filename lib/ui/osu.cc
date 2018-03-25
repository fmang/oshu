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

static void draw_hint(oshu::ui::osu &view, struct oshu_hit *hit)
{
	oshu_game *game = &view.game;
	double now = game->clock.now;
	if (hit->time > now && hit->state == OSHU_INITIAL_HIT) {
		SDL_SetRenderDrawColor(view.display->renderer, 255, 128, 64, 255);
		double ratio = (double) (hit->time - now) / game->beatmap.difficulty.approach_time;
		double base_radius = game->beatmap.difficulty.circle_radius;
		double radius = base_radius + ratio * game->beatmap.difficulty.approach_size;
		oshu_draw_scaled_texture(
			view.display, &view.approach_circle, hit->p,
			2. * radius / std::real(view.approach_circle.size)
		);
	}
}

static void draw_hit_mark(oshu::ui::osu &view, struct oshu_hit *hit)
{
	oshu_game *game = &view.game;
	if (hit->state == OSHU_GOOD_HIT) {
		double leniency = game->beatmap.difficulty.leniency;
		struct oshu_texture *mark = &view.good_mark;
		if (hit->offset < - leniency / 2)
			mark = &view.early_mark;
		else if (hit->offset > leniency / 2)
			mark = &view.late_mark;
		oshu_draw_texture(view.display, mark, oshu_end_point(hit));
	} else if (hit->state == OSHU_MISSED_HIT) {
		oshu_draw_texture(view.display, &view.bad_mark, oshu_end_point(hit));
	} else if (hit->state == OSHU_SKIPPED_HIT) {
		oshu_draw_texture(view.display, &view.skip_mark, oshu_end_point(hit));
	}
}

static void draw_hit_circle(oshu::ui::osu &view, struct oshu_hit *hit)
{
	oshu_game *game = &view.game;
	struct oshu_display *display = view.display;
	if (hit->state == OSHU_INITIAL_HIT) {
		assert (hit->color != NULL);
		oshu_draw_texture(display, &view.circles[hit->color->index], hit->p);
		draw_hint(view, hit);
	} else {
		draw_hit_mark(view, hit);
	}
}

static void draw_slider(oshu::ui::osu &view, struct oshu_hit *hit)
{
	oshu_game *game = &view.game;
	struct oshu_display *display = view.display;
	double now = game->clock.now;
	if (hit->state == OSHU_INITIAL_HIT || hit->state == OSHU_SLIDING_HIT) {
		if (!hit->texture) {
			osu_paint_slider(view, hit);
			assert (hit->texture != NULL);
		}
		oshu_draw_texture(view.display, hit->texture, hit->p);
		draw_hint(view, hit);
		/* ball */
		double t = (now - hit->time) / hit->slider.duration;
		if (hit->state == OSHU_SLIDING_HIT) {
			oshu_point ball = oshu_path_at(&hit->slider.path, t < 0 ? 0 : t);
			oshu_draw_texture(display, &view.slider_ball, ball);
		}
	} else {
		draw_hit_mark(view, hit);
	}
}

static void draw_hit(oshu::ui::osu &view, struct oshu_hit *hit)
{
	oshu_game *game = &view.game;
	if (hit->type & OSHU_SLIDER_HIT)
		draw_slider(view, hit);
	else if (hit->type & OSHU_CIRCLE_HIT)
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
static void connect_hits(oshu::ui::osu &view, struct oshu_hit *a, struct oshu_hit *b)
{
	oshu_game *game = &view.game;
	if (a->state != OSHU_INITIAL_HIT && a->state != OSHU_SLIDING_HIT)
		return;
	oshu_point a_end = oshu_end_point(a);
	double radius = game->beatmap.difficulty.circle_radius;
	double interval = 15;
	double center_distance = std::abs(b->p - a_end);
	double edge_distance = center_distance - 2 * radius;
	if (edge_distance < interval)
		return;
	int steps = edge_distance / interval;
	assert (steps >= 1);
	interval = edge_distance / steps; /* recalibrate */
	oshu_vector direction = (b->p - a_end) / center_distance;
	oshu_point start = a_end + direction * radius;
	oshu_vector step = direction * interval;
	for (int i = 0; i < steps; ++i)
		oshu_draw_texture(view.display, &view.connector, start + (i + .5) * step);
}

namespace oshu {
namespace ui {

/**
 * \todo
 * Handle errors.
 */
osu::osu(oshu_display *display, osu_game &game)
: display(display), game(game)
{
	assert (display != nullptr);
	osu_view(display);
	int rc = osu_paint_resources(*this);
	if (oshu_create_cursor(display, &cursor) < 0)
		rc = -1;
	oshu_reset_view(display);
	mouse = std::make_shared<osu_mouse>(display);
	game.osu.mouse = mouse;
}

osu::~osu()
{
	SDL_ShowCursor(SDL_ENABLE);
	osu_free_resources(*this);
	oshu_destroy_cursor(&cursor);
}

/**
 * Draw all the visible nodes from the beatmap, according to the current
 * position in the song.
 */
void osu::draw()
{
	osu_view(display);
	struct oshu_hit *cursor = oshu_look_hit_up(&game, game.beatmap.difficulty.approach_time);
	struct oshu_hit *next = NULL;
	double now = game.clock.now;
	for (struct oshu_hit *hit = cursor; hit; hit = hit->previous) {
		if (!(hit->type & (OSHU_CIRCLE_HIT | OSHU_SLIDER_HIT)))
			continue;
		if (oshu_hit_end_time(hit) < now - game.beatmap.difficulty.approach_time)
			break;
		if (next && next->combo == hit->combo)
			connect_hits(*this, hit, next);
		draw_hit(*this, hit);
		next = hit;
	}
	oshu_show_cursor(&this->cursor);
	oshu_reset_view(display);
}

osu_mouse::osu_mouse(oshu_display *display)
: display(display)
{
}

oshu_point osu_mouse::position()
{
	osu_view(display);
	oshu_point mouse = oshu_get_mouse(display);
	oshu_reset_view(display);
	return mouse;
}

}}

void osu_view(struct oshu_display *display)
{
	oshu_fit_view(&display->view, oshu_size{640, 480});
	oshu_resize_view(&display->view, oshu_size{512, 384});
}

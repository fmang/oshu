/**
 * \file game/osu/osu.c
 * \ingroup osu
 *
 * Implement the osu! standard mode.
 */

#include "game/game.h"

#include <assert.h>
#include <cairo/cairo.h>

/**
 * Find the first clickable hit object that contains the given x/y coordinates.
 *
 * A hit object is clickable if it is close enough in time and not already
 * clicked.
 *
 * If two hit objects overlap, yield the oldest unclicked one.
 */
static struct oshu_hit* find_hit(struct oshu_game *game, oshu_point p)
{
	struct oshu_hit *start = oshu_look_hit_back(game, game->beatmap.difficulty.approach_time);
	double max_time = game->clock.now + game->beatmap.difficulty.approach_time;
	for (struct oshu_hit *hit = start; hit->time <= max_time; hit = hit->next) {
		if (!(hit->type & (OSHU_CIRCLE_HIT | OSHU_SLIDER_HIT)))
			continue;
		if (hit->state != OSHU_INITIAL_HIT)
			continue;
		if (oshu_distance(p, hit->p) <= game->beatmap.difficulty.circle_radius)
			return hit;
	}
	return NULL;
}

/**
 * Release the held slider, either because the held key is released, or because
 * a new slider is activated (somehow).
 */
static void release_slider(struct oshu_game *game)
{
	struct oshu_hit *hit = game->osu.current_slider;
	if (!hit)
		return;
	assert (hit->type & OSHU_SLIDER_HIT);
	if (game->clock.now < oshu_hit_end_time(hit) - game->beatmap.difficulty.leniency) {
		hit->state = OSHU_MISSED_HIT;
	} else {
		hit->state = OSHU_GOOD_HIT;
		oshu_play_sound(&game->library, &hit->slider.sounds[hit->slider.repeat], &game->audio);
	}
	oshu_stop_loop(&game->audio);
	game->osu.current_slider = NULL;
}

/**
 * Make the sliders emit a sound when reaching an end point.
 *
 * When the last one is reached, release the slider.
 */
static void sonorize_slider(struct oshu_game *game)
{
	struct oshu_hit *hit = game->osu.current_slider;
	if (!hit)
		return;
	assert (hit->type & OSHU_SLIDER_HIT);
	int t = (game->clock.now - hit->time) / hit->slider.duration;
	int prev_t = (game->clock.before - hit->time) / hit->slider.duration;
	if (game->clock.now > oshu_hit_end_time(hit)) {
		release_slider(game);
	} else if (t > prev_t && prev_t >= 0) {
		assert (t <= hit->slider.repeat);
		oshu_play_sound(&game->library, &hit->slider.sounds[t], &game->audio);
	}
}

/**
 * Get the audio position and deduce events from it.
 *
 * For example, when the audio is past a hit object and beyond the threshold of
 * tolerance, mark that hit as missed.
 *
 * Also mark sliders as missed if the mouse is too far from where it's supposed
 * to be.
 */
static int check(struct oshu_game *game)
{
	/* Ensure the mouse follows the slider. */
	sonorize_slider(game); /* < may release the slider! */
	if (game->osu.current_slider) {
		struct oshu_hit *hit = game->osu.current_slider;
		double t = (game->clock.now - hit->time) / hit->slider.duration;
		oshu_point ball = oshu_path_at(&hit->slider.path, t);
		oshu_point mouse = oshu_get_mouse(&game->display);
		if (oshu_distance(ball, mouse) > game->beatmap.difficulty.slider_tolerance) {
			oshu_stop_loop(&game->audio);
			game->osu.current_slider = NULL;
			hit->state = OSHU_MISSED_HIT;
		}
	}
	/* Mark dead notes as missed. */
	double left_wall = game->clock.now - game->beatmap.difficulty.leniency;
	while (game->hit_cursor->time < left_wall) {
		struct oshu_hit *hit = game->hit_cursor;
		if (!(hit->type & (OSHU_CIRCLE_HIT | OSHU_SLIDER_HIT)))
			hit->state = OSHU_UNKNOWN_HIT;
		else if (hit->state == OSHU_INITIAL_HIT)
			hit->state = OSHU_MISSED_HIT;
		game->hit_cursor = hit->next;
	}
	return 0;
}

/**
 * Mark a hit as active.
 *
 * For a circle hit, mark it as good. For a slider, mark it as sliding.
 * Unknown hits are marked as unknown.
 *
 * The key is the held key, relevant only for sliders. In autoplay mode, it's
 * value doesn't matter.
 */
static void activate_hit(struct oshu_game *game, struct oshu_hit *hit, enum oshu_key key)
{
	if (hit->type & OSHU_SLIDER_HIT) {
		release_slider(game);
		hit->state = OSHU_SLIDING_HIT;
		game->osu.current_slider = hit;
		game->osu.held_key = key;
		oshu_play_sound(&game->library, &hit->sound, &game->audio);
		oshu_play_sound(&game->library, &hit->slider.sounds[0], &game->audio);
	} else if (hit->type & OSHU_CIRCLE_HIT) {
		hit->state = OSHU_GOOD_HIT;
		oshu_play_sound(&game->library, &hit->sound, &game->audio);
	} else {
		hit->state = OSHU_UNKNOWN_HIT;
	}
}

/**
 * Automatically activate the sliders on time.
 */
static int autoplay(struct oshu_game *game)
{
	sonorize_slider(game);
	while (game->hit_cursor->time < game->clock.now) {
		activate_hit(game, game->hit_cursor, OSHU_UNKNOWN_KEY);
		game->hit_cursor = game->hit_cursor->next;
	}
	return 0;
}

/**
 * Get the current mouse position, get the hit object, and change its state.
 *
 * Play a sample depending on what was clicked, and when.
 */
static int press(struct oshu_game *game, enum oshu_key key)
{
	oshu_point mouse = oshu_get_mouse(&game->display);
	struct oshu_hit *hit = find_hit(game, mouse);
	if (!hit)
		return 0;
	if (fabs(hit->time - game->clock.now) < game->beatmap.difficulty.leniency) {
		activate_hit(game, hit, key);
	} else {
		hit->state = OSHU_MISSED_HIT;
	}
	return 0;
}

/**
 * When the user is holding a slider or a hold note in mania mode, release the
 * thing.
 */
static int release(struct oshu_game *game, enum oshu_key key)
{
	if (game->osu.held_key == key)
		release_slider(game);
	return 0;
}

static int relinquish(struct oshu_game *game)
{
	if (game->osu.current_slider) {
		game->osu.current_slider->state = OSHU_INITIAL_HIT;
		oshu_stop_loop(&game->audio);
		game->osu.current_slider = NULL;
	}
	return 0;
}

static int adjust(struct oshu_game *game)
{
	oshu_reset_view(&game->display);
	oshu_fit_view(&game->display.view, 640, 480);
	oshu_resize_view(&game->display.view, 512, 384);
	return 0;
}

static int initialize(struct oshu_game *game)
{
	/* XXX Toying around */
	SDL_Surface *surface = SDL_CreateRGBSurface(
		0, 400, 400, 32,
		0x00FF0000, 0x0000FF00, 0x000000FF, 0xFF000000);
	SDL_LockSurface(surface);
	cairo_surface_t *pad = cairo_image_surface_create_for_data(
		surface->pixels, CAIRO_FORMAT_ARGB32,
		surface->w, surface->h, surface->pitch);
	cairo_t *cr = cairo_create(pad);
	/* Draw */
	double xc = 128.0;
	double yc = 128.0;
	double radius = 100.0;
	double angle1 = 45.0  * (M_PI/180.0);  /* angles are specified */
	double angle2 = 180.0 * (M_PI/180.0);  /* in radians           */

	cairo_set_line_width (cr, 10.0);
	cairo_arc (cr, xc, yc, radius, angle1, angle2);
	cairo_stroke (cr);

	/* draw helping lines */
	cairo_set_source_rgba (cr, 1, 0.2, 0.2, 0.6);
	cairo_set_line_width (cr, 6.0);

	cairo_arc (cr, xc, yc, 10.0, 0, 2*M_PI);
	cairo_fill (cr);

	cairo_arc (cr, xc, yc, radius, angle1, angle1);
	cairo_line_to (cr, xc, yc);
	cairo_arc (cr, xc, yc, radius, angle2, angle2);
	cairo_line_to (cr, xc, yc);
	cairo_stroke (cr);
	/* Finish */
	SDL_UnlockSurface(surface);
	game->osu.circle_texture = SDL_CreateTextureFromSurface(
		game->display.renderer, surface);
	SDL_FreeSurface(surface);
	return 0;
}

static int destroy(struct oshu_game *game)
{
	return 0;
}

struct oshu_game_mode osu_mode = {
	.initialize = initialize,
	.destroy = destroy,
	.check = check,
	.autoplay = autoplay,
	.adjust = adjust,
	.draw = osu_draw,
	.press = press,
	.release = release,
	.relinquish = relinquish,
};

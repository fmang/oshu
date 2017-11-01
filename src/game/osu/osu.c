/**
 * \file game/osu/osu.c
 * \ingroup osu
 *
 * Implement the osu! standard mode.
 */

#include "game/game.h"

#include <assert.h>

/**
 * Find the first clickable hit object that contains the given x/y coordinates.
 *
 * A hit object is clickable if it is close enough in time and not already
 * clicked.
 *
 * If two hit objects overlap, yield the oldest unclicked one.
 */
static struct oshu_hit* find_hit(struct oshu_game *game, struct oshu_point p)
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
		struct oshu_point ball = oshu_path_at(&hit->slider.path, t);
		struct oshu_point mouse = oshu_get_mouse(&game->display);
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
 * \todo
 * Factor the hit actions.
 */
static int autoplay(struct oshu_game *game)
{
	sonorize_slider(game);
	while (game->hit_cursor->time < game->clock.now) {
		struct oshu_hit *hit = game->hit_cursor;
		if (hit->type & OSHU_SLIDER_HIT) {
			hit->state = OSHU_SLIDING_HIT;
			game->osu.current_slider = hit;
			oshu_play_sound(&game->library, &hit->sound, &game->audio);
			oshu_play_sound(&game->library, &hit->slider.sounds[0], &game->audio);
		} else if (hit->type & OSHU_CIRCLE_HIT) {
			hit->state = OSHU_GOOD_HIT;
			oshu_play_sound(&game->library, &hit->sound, &game->audio);
		} else {
			hit->state = OSHU_UNKNOWN_HIT;
		}
		game->hit_cursor = hit->next;
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
	struct oshu_point mouse = oshu_get_mouse(&game->display);
	struct oshu_hit *hit = find_hit(game, mouse);
	if (!hit)
		return 0;
	if (fabs(hit->time - game->clock.now) < game->beatmap.difficulty.leniency) {
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
		}
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

struct oshu_game_mode osu_mode = {
	.check = check,
	.autoplay = autoplay,
	.adjust = adjust,
	.draw = osu_draw,
	.press = press,
	.release = release,
	.relinquish = relinquish,
};

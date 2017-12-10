/**
 * \file osu/osu.c
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
static struct oshu_hit* find_hit(struct oshu_game *game, oshu_point p)
{
	struct oshu_hit *start = oshu_look_hit_back(game, game->beatmap.difficulty.approach_time);
	double max_time = game->clock.now + game->beatmap.difficulty.approach_time;
	for (struct oshu_hit *hit = start; hit->time <= max_time; hit = hit->next) {
		if (!(hit->type & (OSHU_CIRCLE_HIT | OSHU_SLIDER_HIT)))
			continue;
		if (hit->state != OSHU_INITIAL_HIT)
			continue;
		if (cabs(p - hit->p) <= game->beatmap.difficulty.circle_radius)
			return hit;
	}
	return NULL;
}

/**
 * Free the texture associated to a slider, when the slider gets old.
 *
 * It is unlikely that once a slider is marked as good or missed, it will ever
 * be shown once again. The main exception is when the user seeks.
 */
static void jettison_hit(struct oshu_hit *hit)
{
	if (hit->texture) {
		oshu_destroy_texture(hit->texture);
		free(hit->texture);
		hit->texture = NULL;
	}
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
	jettison_hit(hit);
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
	osu_view(game);
	/* Ensure the mouse follows the slider. */
	sonorize_slider(game); /* < may release the slider! */
	if (game->osu.current_slider) {
		struct oshu_hit *hit = game->osu.current_slider;
		double t = (game->clock.now - hit->time) / hit->slider.duration;
		oshu_point ball = oshu_path_at(&hit->slider.path, t);
		oshu_point mouse = oshu_get_mouse(&game->display);
		if (cabs(ball - mouse) > game->beatmap.difficulty.slider_tolerance) {
			oshu_stop_loop(&game->audio);
			game->osu.current_slider = NULL;
			hit->state = OSHU_MISSED_HIT;
			jettison_hit(hit);
		}
	}
	/* Mark dead notes as missed. */
	double left_wall = game->clock.now - game->beatmap.difficulty.leniency;
	while (game->hit_cursor->time < left_wall) {
		struct oshu_hit *hit = game->hit_cursor;
		if (!(hit->type & (OSHU_CIRCLE_HIT | OSHU_SLIDER_HIT))) {
			hit->state = OSHU_UNKNOWN_HIT;
		} else if (hit->state == OSHU_INITIAL_HIT) {
			hit->state = OSHU_MISSED_HIT;
			jettison_hit(hit);
		}
		game->hit_cursor = hit->next;
	}
	oshu_reset_view(&game->display);
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
static void activate_hit(struct oshu_game *game, struct oshu_hit *hit, enum oshu_finger key)
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
static int press(struct oshu_game *game, enum oshu_finger key)
{
	osu_view(game);
	oshu_point mouse = oshu_get_mouse(&game->display);
	oshu_reset_view(&game->display);
	struct oshu_hit *hit = find_hit(game, mouse);
	if (!hit)
		return 0;
	if (fabs(hit->time - game->clock.now) < game->beatmap.difficulty.leniency) {
		activate_hit(game, hit, key);
		hit->offset = game->clock.now - hit->time;
	} else {
		hit->state = OSHU_MISSED_HIT;
		jettison_hit(hit);
	}
	return 0;
}

/**
 * When the user is holding a slider or a hold note in mania mode, release the
 * thing.
 */
static int release(struct oshu_game *game, enum oshu_finger key)
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

static int init(struct oshu_game *game)
{
	oshu_reset_view(&game->display);
	oshu_fit_view(&game->display.view, 640 + 480 * I);
	int rc = osu_paint_resources(game);
	SDL_ShowCursor(SDL_DISABLE);
	return rc;
}

static int destroy(struct oshu_game *game)
{
	SDL_ShowCursor(SDL_ENABLE);
	osu_free_resources(game);
	return 0;
}

void osu_view(struct oshu_game *game)
{
	oshu_fit_view(&game->display.view, 640 + 480 * I);
	oshu_resize_view(&game->display.view, 512 + 384 * I);
}

struct oshu_game_mode osu_mode = {
	.initialize = init,
	.destroy = destroy,
	.check = check,
	.autoplay = autoplay,
	.draw = osu_draw,
	.press = press,
	.release = release,
	.relinquish = relinquish,
};

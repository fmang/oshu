/**
 * \file game/osu/osu.c
 * \ingroup osu
 *
 * Implement the osu! standard mode.
 */

#include "game/game.h"
#include "graphics/draw.h"

/**
 * Find the first clickable hit object that contains the given x/y coordinates.
 *
 * A hit object is clickable if it is close enough in time and not already
 * clicked.
 *
 * If two hit objects overlap, yield the oldest unclicked one.
 *
 * \todo
 * Simplify the code using the new properties of the hits list.
 *
 * \todo
 * Use #oshu_look_hit_back for robustness.
 */
static struct oshu_hit* find_hit(struct oshu_game *game, struct oshu_point p)
{
	for (struct oshu_hit *hit = game->hit_cursor; hit; hit = hit->next) {
		if (hit->time > game->clock.now + game->beatmap.difficulty.approach_time)
			break;
		if (hit->time < game->clock.now - game->beatmap.difficulty.approach_time)
			continue;
		if (!(hit->type & (OSHU_CIRCLE_HIT | OSHU_SLIDER_HIT)))
			continue;
		if (hit->state != OSHU_INITIAL_HIT)
			continue;
		double dist = oshu_distance(p, hit->p);
		if (dist <= game->beatmap.difficulty.circle_radius)
			return hit;
	}
	return NULL;
}

/**
 * Get the current mouse position, get the hit object, and change its state.
 *
 * Play a sample depending on what was clicked, and when.
 *
 * \todo
 * Handle the held key.
 *
 * \todo
 * Merge with #press.
 */
static void hit(struct oshu_game *game)
{
	struct oshu_point mouse = oshu_get_mouse(&game->display);
	struct oshu_hit *hit = find_hit(game, mouse);
	if (hit) {
		if (fabs(hit->time - game->clock.now) < game->beatmap.difficulty.leniency) {
			if (hit->type & OSHU_SLIDER_HIT) {
				hit->state = OSHU_SLIDING_HIT;
				game->osu.current_slider = hit;
				oshu_play_sound(&game->library, &hit->sound, &game->audio);
				oshu_play_sound(&game->library, &hit->slider.sounds[0], &game->audio);
			} else if (hit->type & OSHU_CIRCLE_HIT) {
				hit->state = OSHU_GOOD_HIT;
				oshu_play_sound(&game->library, &hit->sound, &game->audio);
			}
		} else {
			hit->state = OSHU_MISSED_HIT;
		}
	}
}

/**
 * When the user is holding a slider or a hold note in mania mode, do
 * something.
 *
 * \todo
 * Rename to `release_slider` for specifity. Add the relevant assertions.
 *
 * \todo
 * Merge with #release.
 */
static void release_hit(struct oshu_game *game)
{
	struct oshu_hit *hit = game->osu.current_slider;
	if (!hit)
		return;
	if (!(hit->type & OSHU_SLIDER_HIT))
		return;
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
 * Check the state of the current slider.
 *
 * Make it end if it's done.
 *
 * \todo
 * This function is too complex, split it.
 */
static void check_slider(struct oshu_game *game)
{
	struct oshu_hit *hit = game->osu.current_slider;
	if (!hit)
		return;
	if (!(hit->type & OSHU_SLIDER_HIT))
		return;
	double t = (game->clock.now - hit->time) / hit->slider.duration;
	double prev_t = (game->clock.before - hit->time) / hit->slider.duration;
	if (game->clock.now > oshu_hit_end_time(hit)) {
		oshu_stop_loop(&game->audio);
		game->osu.current_slider = NULL;
		hit->state = OSHU_GOOD_HIT;
		oshu_play_sound(&game->library, &hit->slider.sounds[hit->slider.repeat], &game->audio);
	} else if ((int) t > (int) prev_t && prev_t > 0) {
		oshu_play_sound(&game->library, &hit->slider.sounds[(int) t], &game->audio);
	}
	if (game->autoplay)
		return;
	struct oshu_point ball = oshu_path_at(&hit->slider.path, t);
	struct oshu_point mouse = oshu_get_mouse(&game->display);
	double dist = oshu_distance(ball, mouse);
	if (dist > game->beatmap.difficulty.slider_tolerance) {
		oshu_stop_loop(&game->audio);
		game->osu.current_slider = NULL;
		hit->state = OSHU_MISSED_HIT;
	}
}

/**
 * Get the audio position and deduce events from it.
 *
 * For example, when the audio is past a hit object and beyond the threshold of
 * tolerance, mark that hit as missed.
 */
static void check_audio(struct oshu_game *game)
{
	double left_wall = game->clock.now - game->beatmap.difficulty.leniency;
	while (game->hit_cursor->time < left_wall) {
		struct oshu_hit *hit = game->hit_cursor;
		/* Mark dead notes as missed. */
		if (!(hit->type & (OSHU_CIRCLE_HIT | OSHU_SLIDER_HIT)))
			hit->state = OSHU_UNKNOWN_HIT;
		else if (hit->state == OSHU_INITIAL_HIT)
			hit->state = OSHU_MISSED_HIT;
		game->hit_cursor = hit->next;
	}
}

/**
 * \todo
 * Merge with #check_audio maybe.
 * For sliders, see how it can be factor with autoplay.
 */
static int check(struct oshu_game *game)
{
	check_slider(game);
	check_audio(game);
	return 0;
}

/**
 * \todo
 * Factor the slider logic.
 *
 * \todo
 * Factor the hit actions.
 */
static int autoplay(struct oshu_game *game)
{
	check_slider(game);
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
 * \todo
 * Export to the draw sub-module.
 */
static int draw(struct oshu_game *game)
{
	struct oshu_hit *start = oshu_look_hit_back(game, game->beatmap.difficulty.approach_time);
	osu_draw_beatmap(&game->display, &game->beatmap, start, game->clock.now);
	return 0;
}

/**
 * \todo
 * Update game->osu.held_key for sliders.
 *
 * \todo
 * Merge with #hit.
 */
static int press(struct oshu_game *game, enum oshu_key key)
{
	hit(game);
	return 0;
}

/**
 * \todo
 * Check key == game->osu.held_key
 *
 * \todo
 * Merge with #release_hit.
 */
static int release(struct oshu_game *game, enum oshu_key key)
{
	release_hit(game);
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
	.draw = draw,
	.press = press,
	.release = release,
	.relinquish = relinquish,
};

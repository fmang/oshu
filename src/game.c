/**
 * \file game.c
 * \ingroup game
 *
 * Game module implementation.
 */

#include "log.h"
#include "game.h"

#include <unistd.h>

int oshu_game_create(const char *beatmap_path, struct oshu_game **game)
{
	*game = calloc(1, sizeof(**game));

	if (oshu_beatmap_load(beatmap_path, &(*game)->beatmap) < 0) {
		oshu_log_error("no beatmap, aborting");
		goto fail;
	}
	if ((*game)->beatmap->mode != OSHU_MODE_OSU) {
		oshu_log_error("unsupported game mode");
		goto fail;
	}

	if (oshu_audio_open((*game)->beatmap->audio_filename, &(*game)->audio) < 0) {
		oshu_log_error("no audio, aborting");
		goto fail;
	}

	const char *hit_path = "hit.wav";
	if (access(hit_path, F_OK) != 0)
		hit_path = PKGDATADIR "/hit.wav";
	oshu_log_debug("loading %s", hit_path);
	if (oshu_sample_load(hit_path, (*game)->audio, &(*game)->hit_sound) < 0) {
		oshu_log_error("could not load hit.wav, aborting");
		goto fail;
	}

	if (oshu_display_init(&(*game)->display) < 0) {
		oshu_log_error("no display, aborting");
		goto fail;
	}

	return 0;
fail:
	oshu_game_destroy(game);
	return -1;
}

/**
 * Find the first clickable hit object that contains the given x/y coordinates.
 *
 * A hit object is clickable if it is close enough in time and not already
 * clicked.
 *
 * If two hit objects overlap, yield the oldest unclicked one.
 */
static struct oshu_hit* find_hit(struct oshu_game *game, int x, int y)
{
	double now = game->audio->current_timestamp;
	for (struct oshu_hit *hit = game->beatmap->hit_cursor; hit; hit = hit->next) {
		if (hit->time > now + game->beatmap->difficulty.approach_time)
			break;
		if (hit->time < now - game->beatmap->difficulty.approach_time)
			continue;
		if (hit->state != OSHU_HIT_INITIAL)
			continue;
		int dx = x - hit->x;
		int dy = y - hit->y;
		int dist = sqrt(dx * dx + dy * dy);
		if (dist <= game->beatmap->difficulty.circle_radius)
			return hit;
	}
	return NULL;
}

/**
 * Get the current mouse position, get the hit object, and change its state.
 *
 * Play a sample depending on what was clicked, and when.
 */
static void hit(struct oshu_game *game)
{
	if (game->paused || game->autoplay)
		return;
	int x, y;
	oshu_get_mouse(game->display, &x, &y);
	double now = game->audio->current_timestamp;
	struct oshu_hit *hit = find_hit(game, x, y);
	if (hit) {
		if (fabs(hit->time - now) < game->beatmap->difficulty.leniency) {
			if (hit->type & OSHU_HIT_SLIDER && hit->slider.path.type) {
				hit->state = OSHU_HIT_SLIDING;
				game->current_hit = hit;
			} else {
				hit->state = OSHU_HIT_GOOD;
			}
			oshu_sample_play(game->audio, game->hit_sound);
		} else {
			hit->state = OSHU_HIT_MISSED;
		}
	}
}

/**
 * When the user is holding a slider or a hold note in mania mode, do
 * something.
 */
static void release_hit(struct oshu_game *game)
{
	struct oshu_hit *hit = game->current_hit;
	if (!hit)
		return;
	if (!(hit->type & OSHU_HIT_SLIDER))
		return;
	double now = game->audio->current_timestamp;
	if (now < oshu_hit_end_time(hit) - game->beatmap->difficulty.leniency) {
		hit->state = OSHU_HIT_MISSED;
	} else {
		hit->state = OSHU_HIT_GOOD;
		oshu_sample_play(game->audio, game->hit_sound);
	}
	game->current_hit = NULL;
}

static void pause_game(struct oshu_game *game)
{
	oshu_audio_pause(game->audio);
	game->paused = 1;
}

static void toggle_pause(struct oshu_game *game)
{
	if (game->paused) {
		oshu_audio_play(game->audio);
		game->paused = 0;
	} else {
		pause_game(game);
	}
}

/**
 * React to an event got from SDL.
 */
static void handle_event(struct oshu_game *game, SDL_Event *event)
{
	switch (event->type) {
	case SDL_KEYDOWN:
		if (event->key.repeat)
			break;
		switch (event->key.keysym.sym) {
		case SDLK_w:
		case SDLK_x:
		case SDLK_z:
			hit(game);
			break;
		case SDLK_q:
			game->stop = 1;
			break;
		case SDLK_ESCAPE:
		case SDLK_SPACE:
			toggle_pause(game);
			break;
		}
		break;
	case SDL_KEYUP:
		switch (event->key.keysym.sym) {
		case SDLK_w:
		case SDLK_x:
		case SDLK_z:
			release_hit(game);
			break;
		}
		break;
	case SDL_MOUSEBUTTONDOWN:
		hit(game);
		break;
	case SDL_MOUSEBUTTONUP:
		release_hit(game);
		break;
	case SDL_WINDOWEVENT:
		switch (event->window.event) {
		case SDL_WINDOWEVENT_SIZE_CHANGED:
			oshu_display_resize(game->display, event->window.data1, event->window.data2);
			break;
		case SDL_WINDOWEVENT_MINIMIZED:
		case SDL_WINDOWEVENT_FOCUS_LOST:
			pause_game(game);
			break;
		case SDL_WINDOWEVENT_CLOSE:
			game->stop = 1;
			break;
		}
		break;
	}
}

/**
 * Check the state of the current slider.
 *
 * Make it end if it's done.
 */
static void check_slider(struct oshu_game *game)
{
	struct oshu_hit *hit = game->current_hit;
	if (!hit)
		return;
	if (!(hit->type & OSHU_HIT_SLIDER))
		return;
	double now = game->audio->current_timestamp;
	double t = (now - hit->time) / hit->slider.duration;
	double prev_t = (game->previous_time - hit->time) / hit->slider.duration;
	if (now > oshu_hit_end_time(hit)) {
		game->current_hit = NULL;
		hit->state = OSHU_HIT_GOOD;
		oshu_sample_play(game->audio, game->hit_sound);
	} else if ((int) t > (int) prev_t && prev_t > 0) {
		oshu_sample_play(game->audio, game->hit_sound);
	}
	if (game->autoplay)
		return;
	struct oshu_point ball = oshu_path_at(&hit->slider.path, t);
	int x, y;
	oshu_get_mouse(game->display, &x, &y);
	int dx = x - ball.x;
	int dy = y - ball.y;
	int dist = sqrt(dx * dx + dy * dy);
	if (dist > game->beatmap->difficulty.slider_tolerance) {
		game->current_hit = NULL;
		hit->state = OSHU_HIT_MISSED;
	}
}

/**
 * Get the audio position and deduce events from it.
 *
 * For example, when the audio is past a hit object and beyond the threshold of
 * tolerance, mark that hit as missed.
 *
 * Also move the beatmap's hit cursor to optimize the beatmap manipulation
 * routines.
 */
static void check_audio(struct oshu_game *game)
{
	double now = game->audio->current_timestamp;
	if (game->autoplay) {
		for (struct oshu_hit *hit = game->beatmap->hit_cursor; hit; hit = hit->next) {
			if (hit->time > now) {
				break;
			} else if (hit->state == OSHU_HIT_INITIAL) {
				if (hit->type & OSHU_HIT_SLIDER && hit->slider.path.type) {
					hit->state = OSHU_HIT_SLIDING;
					game->current_hit = hit;
				} else {
					hit->state = OSHU_HIT_GOOD;
				}
				oshu_sample_play(game->audio, game->hit_sound);
			}
		}
	} else {
		/* Mark dead notes as missed. */
		for (struct oshu_hit *hit = game->beatmap->hit_cursor; hit; hit = hit->next) {
			if (hit->time > now - game->beatmap->difficulty.leniency)
				break;
			if (hit->state == OSHU_HIT_INITIAL)
				hit->state = OSHU_HIT_MISSED;
		}
	}
	for (
		struct oshu_hit **hit = &game->beatmap->hit_cursor;
		*hit && oshu_hit_end_time(*hit) < now - game->beatmap->difficulty.approach_time;
		*hit = (*hit)->next
	);
}

/**
 * Print the score if the song was finished, then exit.
 */
static void end(struct oshu_game *game)
{
	if (!game->audio->finished)
		return;
	int good = 0;
	int missed = 0;
	for (struct oshu_hit *hit = game->beatmap->hits; hit; hit = hit->next) {
		if (hit->state == OSHU_HIT_MISSED)
			missed++;
		else if (hit->state == OSHU_HIT_GOOD)
			good++;
	}
	printf(
		"\n"
		"*** Score ***\n"
		"  Good: %d\n"
		"  Miss: %d\n"
		"\n",
		good,
		missed
	);
}

int oshu_game_run(struct oshu_game *game)
{
	SDL_Event event;
	if (!game->paused)
		oshu_audio_play(game->audio);
	while (!game->audio->finished && !game->stop) {
		while (SDL_PollEvent(&event))
			handle_event(game, &event);
		check_slider(game);
		check_audio(game);
		double now = game->audio->current_timestamp;
		oshu_draw_beatmap(game->display, game->beatmap, now);
		game->previous_time = now;
		SDL_Delay(20);
	}
	end(game);
	return 0;
}

void oshu_game_destroy(struct oshu_game **game)
{
	if (!game)
		return;
	if (!*game)
		return;
	if ((*game)->audio)
		oshu_audio_close(&(*game)->audio);
	if ((*game)->hit_sound)
		oshu_sample_free(&(*game)->hit_sound);
	if ((*game)->display)
		oshu_display_destroy(&(*game)->display);
	if ((*game)->beatmap)
		oshu_beatmap_free(&(*game)->beatmap);
	free(*game);
	*game = NULL;
}

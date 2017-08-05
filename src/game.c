/**
 * \file game.c
 * \ingroup game
 *
 * \ref game module implementation.
 */

#include "log.h"
#include "game.h"

/** How long before or after the hit time a click event is considered on the
 *  mark. */
static int hit_tolerance = 100 ; /* ms */

/** How long before or after the hit time a hit object is clickable. */
static int hit_clickable = 1000 ; /* ms */

int oshu_game_create(const char *beatmap_path, struct oshu_game **game)
{
	*game = calloc(1, sizeof(**game));

	if (oshu_beatmap_load(beatmap_path, &(*game)->beatmap) < 0) {
		oshu_log_error("no beatmap, aborting");
		goto fail;
	}
	if ((*game)->beatmap->general.mode != OSHU_MODE_OSU) {
		oshu_log_error("unsupported game mode");
		goto fail;
	}

	if (oshu_audio_open((*game)->beatmap->general.audio_filename, &(*game)->audio) < 0) {
		oshu_log_error("no audio, aborting");
		goto fail;
	}

	if (oshu_sample_load("hit.wav", (*game)->audio, &(*game)->hit_sound) < 0) {
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
	int now = game->audio->current_timestamp * 1000;
	for (struct oshu_hit *hit = game->beatmap->hit_cursor; hit; hit = hit->next) {
		if (hit->time > now + hit_clickable)
			break;
		if (hit->time < now - hit_clickable)
			continue;
		if (hit->state != OSHU_HIT_INITIAL)
			continue;
		int dx = x - hit->x;
		int dy = y - hit->y;
		int dist = sqrt(dx * dx + dy * dy);
		if (dist <= oshu_hit_radius)
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
	if (game->paused)
		return;
	int x, y;
	oshu_get_mouse(game->display, &x, &y);
	int now = game->audio->current_timestamp * 1000;
	struct oshu_hit *hit = find_hit(game, x, y);
	if (hit) {
		if (abs(hit->time - now) < hit_tolerance) {
			hit->state = OSHU_HIT_GOOD;
			SDL_LockAudio();
			oshu_sample_play(game->audio, game->hit_sound);
			SDL_UnlockAudio();
		} else {
			hit->state = OSHU_HIT_MISSED;
		}
	}
}

static void toggle_pause(struct oshu_game *game)
{
	SDL_LockAudio();
	if (game->paused) {
		oshu_audio_play(game->audio);
		game->paused = 0;
	} else {
		oshu_audio_pause(game->audio);
		game->paused = 1;
	}
	SDL_UnlockAudio();
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
		if (event->key.keysym.sym == SDLK_w)
			hit(game);
		else if (event->key.keysym.sym == SDLK_x)
			hit(game);
		else if (event->key.keysym.sym == SDLK_q)
			game->stop = 1;
		else if (event->key.keysym.sym == SDLK_ESCAPE)
			toggle_pause(game);
		break;
	case SDL_MOUSEBUTTONDOWN:
		hit(game);
		break;
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
	int now = game->audio->current_timestamp * 1000;
	for (struct oshu_hit *hit = game->beatmap->hit_cursor; hit; hit = hit->next) {
		if (hit->time > now - hit_tolerance)
			break;
		if (hit->state == OSHU_HIT_INITIAL)
			hit->state = OSHU_HIT_MISSED;
	}
	for (
		struct oshu_hit **hit = &game->beatmap->hit_cursor;
		*hit && (*hit)->time < now - hit_clickable;
		*hit = (*hit)->next
	);
}

int oshu_game_run(struct oshu_game *game)
{
	SDL_Event event;
	oshu_audio_play(game->audio);
	while (!game->audio->finished && !game->stop) {
		while (SDL_PollEvent(&event))
			handle_event(game, &event);
		check_audio(game);
		int now = game->audio->current_timestamp * 1000;
		oshu_draw_beatmap(game->display, game->beatmap, now);
		SDL_Delay(20);
	}
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
